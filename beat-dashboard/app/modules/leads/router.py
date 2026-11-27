"""/api/leads/* - lead finder + mini CRM."""
from __future__ import annotations

import asyncio
import csv
import io
import json
import re
from datetime import date, timedelta

from fastapi import APIRouter, File, HTTPException, UploadFile
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field

from ... import db, jobs
from ...config import settings
from .services import artists, enrich, osm, pipeline

router = APIRouter()

FIELDS = ["name", "instagram", "email", "phone", "website", "address", "district", "category", "status", "followers",
          "monthly_listeners", "tone", "notes", "deal_value_est", "deal_value_final", "lost_reason", "next_action_date",
          "starred", "archived", "kind"]
JSON_FIELDS = ("flags", "extra")

COMPLIANCE = {
    "email": "⚖️ Österreich (§174 TKG): Werbe-Mails brauchen vorher eine Einwilligung – auch bei Firmen und office@-Adressen. "
             "Erst persönlich/per Brief fragen („Darf ich Ihnen Beispiele mailen?“), dann mailen.",
    "phone": "⚖️ §174 TKG: Werbeanrufe ohne vorherige Einwilligung sind verboten – auch B2B. Telefon nur für Rückrufe "
             "oder wenn das Lokal dich darum gebeten hat.",
    "instagram": "Grauzone: nur persönliche 1:1-Nachrichten, keine Massen-DMs. Vorher folgen & interagieren, "
                 "max. ~15 neue DMs pro Tag, kein Link in der ersten Nachricht. Bei „kein Interesse“ → Sperrliste.",
    "visit": "✅ Persönlich vorbeischauen ist nicht vom TKG erfasst – der beste Weg bei Restaurants. Di–Do 14:30–17:00.",
    "letter": "✅ Persönlich adressierter Brief / Postkarte mit QR-Code zum Portfolio ist erlaubt.",
    "gdpr": "DSGVO: Bei Personen (Artist, Inhaber) im Erstkontakt kurz sagen, woher du die Daten hast, und dass sie "
            "jederzeit widersprechen können. Leads ohne Ergebnis nach 12 Monaten löschen.",
}


def me() -> dict:
    return {"name": settings.get("MY_NAME") or settings.get("PRODUCER_NAME"), "bezirk": settings.get("MY_BEZIRK"),
            "phone": settings.get("MY_PHONE"), "portfolio": settings.get("PORTFOLIO_LINK")}


def _row(r) -> dict:
    d = db.row_to_dict(r, JSON_FIELDS)
    for f in JSON_FIELDS:
        if not isinstance(d.get(f), dict):
            d[f] = {}
    return d


def _get(c, lid: int) -> dict:
    r = c.execute("SELECT * FROM leads WHERE id=?", (lid,)).fetchone()
    if not r:
        raise HTTPException(404, "Lead nicht gefunden")
    return _row(r)


def _log(c, lid: int, type_: str, text: str = "", channel: str | None = None):
    c.execute("INSERT INTO lead_activities(lead_id, type, channel, text) VALUES(?,?,?,?)", (lid, type_, channel, text))


def _suppressed(c, lead: dict) -> bool:
    for col in ("instagram", "email"):
        if lead.get(col):
            r = c.execute(f"SELECT 1 FROM leads WHERE {col}=? AND json_extract(flags,'$.do_not_contact')=1", (lead[col],)).fetchone()
            if r:
                return True
    return False


def upsert(c, lead: dict) -> str:
    """Insert a found lead or fill gaps of an existing one. Returns new | updated | same | blocked."""
    lead = {k: v for k, v in lead.items() if v not in (None, "")}
    lead.setdefault("kind", "other")
    existing = None
    if lead.get("source") and lead.get("source_id"):
        existing = c.execute("SELECT * FROM leads WHERE kind=? AND source=? AND source_id=?",
                             (lead["kind"], lead["source"], lead["source_id"])).fetchone()
    for col in ("instagram", "email", "website"):
        if existing is None and lead.get(col):
            existing = c.execute(f"SELECT * FROM leads WHERE kind=? AND lower({col})=lower(?)", (lead["kind"], lead[col])).fetchone()
    if existing is None and lead.get("kind") == "artist":
        existing = c.execute("SELECT * FROM leads WHERE kind='artist' AND lower(name)=lower(?)", (lead["name"],)).fetchone()
    if existing is not None:
        ex = _row(existing)
        changes = {}
        for k in ("instagram", "email", "phone", "website", "address", "district", "category", "lat", "lon"):
            if lead.get(k) and not ex.get(k):
                changes[k] = lead[k]
        extra = {**(lead.get("extra") or {}), **ex["extra"]}
        if extra != ex["extra"]:
            changes["extra"] = json.dumps(extra, ensure_ascii=False)
        if not changes:
            return "same"
        merged = {**ex, **changes, "extra": extra}
        changes["score"] = pipeline.score(merged)
        c.execute(f"UPDATE leads SET {', '.join(f'{k}=?' for k in changes)}, updated_at=datetime('now','localtime') WHERE id=?",
                  (*changes.values(), ex["id"]))
        return "updated"
    if _suppressed(c, lead):
        return "blocked"
    lead["score"] = pipeline.score(lead)
    cols = ["kind", "name", "instagram", "email", "phone", "website", "address", "district", "lat", "lon", "category",
            "source", "source_id", "score", "extra", "flags", "followers", "notes"]
    vals = [json.dumps(lead.get(k) or {}, ensure_ascii=False) if k in JSON_FIELDS else lead.get(k) for k in cols]
    c.execute(f"INSERT INTO leads({', '.join(cols)}) VALUES({', '.join('?' * len(cols))})", vals)
    return "new"


_rescored = False


def _seed_templates():
    global _rescored
    if not _rescored:
        _rescored = True
        try:
            rescore()
        except Exception:
            pass
    with db.tx() as c:
        if c.execute("SELECT COUNT(*) FROM lead_templates").fetchone()[0] == 0:
            for i, t in enumerate(pipeline.DEFAULT_TEMPLATES):
                c.execute("INSERT INTO lead_templates(kind, channel, name, subject, body, sort) VALUES(?,?,?,?,?,?)", (*t, i))


# ---------------------------------------------------------------- meta
@router.get("/meta")
def meta():
    _seed_templates()
    return {"stages": {k: [{"id": a, "label": b, "color": c} for a, b, c in v] for k, v in pipeline.STAGES.items()},
            "checks": {k: [{"id": a, "label": b} for a, b in v] for k, v in pipeline.CHECKS.items()},
            "qualifiers": {k: [{"id": a, "label": b, "points": p} for a, b, p in v] for k, v in pipeline.QUALIFIERS.items()},
            "prices": pipeline.PRICES, "areas": osm.areas(), "types": osm.TYPES, "compliance": COMPLIANCE,
            "me": me(), "has_youtube": settings.has("YOUTUBE_API_KEY")}


# ---------------------------------------------------------------- list / CRUD
@router.get("/leads")
def list_leads(kind: str = "", status: str = "", q: str = "", district: str = "", has: str = "",
               sort: str = "score", due: bool = False, archived: bool = False, limit: int = 200, offset: int = 0):
    where, args = ["archived=?"], [int(archived)]
    if kind:
        where.append("kind=?"); args.append(kind)
    if status:
        sts = status.split(",")
        where.append(f"status IN ({','.join('?' * len(sts))})"); args += sts
    if q:
        where.append("(name LIKE ? OR instagram LIKE ? OR email LIKE ? OR category LIKE ? OR address LIKE ? OR notes LIKE ?)")
        args += [f"%{q}%"] * 6
    if district:
        where.append("district=?"); args.append(district)
    for h in filter(None, has.split(",")):
        if h in ("instagram", "email", "phone", "website"):
            where.append(f"COALESCE({h},'')<>''")
    if due:
        where.append("next_action_date IS NOT NULL AND next_action_date<=?"); args.append(date.today().isoformat())
    order = {"score": "starred DESC, score DESC, id DESC", "name": "name COLLATE NOCASE", "next": "next_action_date IS NULL, next_action_date",
             "created": "id DESC", "updated": "updated_at DESC", "district": "district, name"}.get(sort, "score DESC")
    sql_where = " AND ".join(where)
    with db.tx() as c:
        total = c.execute(f"SELECT COUNT(*) FROM leads WHERE {sql_where}", args).fetchone()[0]
        rows = c.execute(f"SELECT * FROM leads WHERE {sql_where} ORDER BY {order} LIMIT ? OFFSET ?", (*args, limit, offset)).fetchall()
        districts = [r[0] for r in c.execute("SELECT DISTINCT district FROM leads WHERE district IS NOT NULL AND kind LIKE ? ORDER BY district",
                                              (kind or "%",)).fetchall()]
    return {"total": total, "items": [_row(r) for r in rows], "districts": districts}


class LeadIn(BaseModel):
    kind: str = "artist"
    name: str
    instagram: str | None = None
    email: str | None = None
    phone: str | None = None
    website: str | None = None
    address: str | None = None
    district: str | None = None
    category: str | None = None
    followers: int | None = None
    notes: str | None = None


@router.post("/leads")
def create_lead(body: LeadIn):
    lead = body.model_dump()
    lead["instagram"] = pipeline.normalize_instagram(lead["instagram"])
    lead["phone"] = pipeline.normalize_phone(lead["phone"])
    lead["source"], lead["source_id"] = "manuell", None
    with db.tx() as c:
        res = upsert(c, lead)
        lid = c.execute("SELECT id FROM leads ORDER BY id DESC LIMIT 1").fetchone()[0] if res == "new" else None
        if lid:
            _log(c, lid, "status", "Lead manuell angelegt")
    if res == "blocked":
        raise HTTPException(409, "Dieser Kontakt steht auf deiner Sperrliste (will nicht kontaktiert werden).")
    return {"result": res, "id": lid}


@router.get("/leads/{lid}")
def get_lead(lid: int):
    with db.tx() as c:
        lead = _get(c, lid)
        acts = c.execute("SELECT * FROM lead_activities WHERE lead_id=? ORDER BY id DESC", (lid,)).fetchall()
    lead["activities"] = [dict(a) for a in acts]
    return lead


class LeadPatch(BaseModel):
    values: dict = Field(default_factory=dict)
    flags: dict = Field(default_factory=dict)


@router.patch("/leads/{lid}")
def patch_lead(lid: int, body: LeadPatch):
    with db.tx() as c:
        lead = _get(c, lid)
        vals = {k: v for k, v in body.values.items() if k in FIELDS}
        if "instagram" in vals:
            vals["instagram"] = pipeline.normalize_instagram(vals["instagram"])
        if "phone" in vals:
            vals["phone"] = pipeline.normalize_phone(vals["phone"])
        for k in ("followers", "monthly_listeners"):
            if k in vals:
                vals[k] = int(re.sub(r"\D", "", str(vals[k])) or 0) if str(vals[k]).strip() else None
        flags = {**lead["flags"], **body.flags}
        if body.flags.get("do_not_contact"):
            vals.setdefault("status", "kein_interesse")
            vals["next_action_date"] = None
        if "status" in vals and vals["status"] != lead["status"]:
            label = dict((a, b) for a, b, _ in pipeline.STAGES.get(lead["kind"], pipeline.STAGES["other"])).get(vals["status"], vals["status"])
            _log(c, lid, "status", f"Status → {label}")
            if vals["status"] == "geantwortet" and not lead.get("replied_at"):
                vals["replied_at"] = date.today().isoformat()
            if vals["status"] in pipeline.CLOSED:
                vals["next_action_date"] = None
        for k, v in body.flags.items():
            if v and not lead["flags"].get(k):
                label = next((b for a, b in pipeline.CHECKS.get(lead["kind"], []) if a == k), None)
                if label:
                    _log(c, lid, "check", f"✓ {label}")
        merged = {**lead, **vals, "flags": flags}
        vals["score"] = pipeline.score(merged)
        vals["flags"] = json.dumps(flags)
        c.execute(f"UPDATE leads SET {', '.join(f'{k}=?' for k in vals)}, updated_at=datetime('now','localtime') WHERE id=?",
                  (*vals.values(), lid))
    return get_lead(lid)


@router.delete("/leads/{lid}")
def delete_lead(lid: int):
    with db.tx() as c:
        c.execute("DELETE FROM leads WHERE id=?", (lid,))
    return {"ok": True}


class ContactIn(BaseModel):
    type: str = "contact"        # contact | reply | note | call | visit | letter
    channel: str = "instagram"
    text: str = ""
    next_days: int | None = None


@router.post("/leads/{lid}/log")
def log_contact(lid: int, body: ContactIn):
    """One click 'angeschrieben' / 'hat geantwortet' / 'Notiz' with automatic status + follow-up date."""
    today = date.today().isoformat()
    with db.tx() as c:
        lead = _get(c, lid)
        upd: dict = {}
        if body.type in ("contact", "call", "visit", "letter"):
            if lead["flags"].get("do_not_contact"):
                raise HTTPException(409, "Steht auf der Sperrliste – nicht mehr kontaktieren.")
            n = (lead.get("contact_count") or 0) + 1
            upd.update(contact_count=n, last_contact_at=today)
            if not lead.get("first_contact_at"):
                upd["first_contact_at"] = today
            first = "angeschrieben" if lead["kind"] == "artist" else "kontaktiert"
            if lead["status"] in ("neu", "warmup", "recherchiert"):
                upd["status"] = first
            elif lead["status"] in (first,) and n >= 2:
                upd["status"] = "followup"
            nxt = (date.today() + timedelta(days=body.next_days)).isoformat() if body.next_days else pipeline.next_followup(lead["kind"], n)
            upd["next_action_date"] = nxt
        elif body.type == "reply":
            upd.update(status="geantwortet", replied_at=today, next_action_date=(date.today() + timedelta(days=1)).isoformat())
        elif body.next_days is not None:
            upd["next_action_date"] = (date.today() + timedelta(days=body.next_days)).isoformat()
        _log(c, lid, body.type, body.text, body.channel)
        if "status" in upd and upd["status"] != lead["status"]:
            _log(c, lid, "status", f"Status → {upd['status']}")
        if upd:
            c.execute(f"UPDATE leads SET {', '.join(f'{k}=?' for k in upd)}, updated_at=datetime('now','localtime') WHERE id=?",
                      (*upd.values(), lid))
    return get_lead(lid)


class BulkIn(BaseModel):
    ids: list[int]
    action: str                 # status | archive | unarchive | delete | star
    value: str | None = None


@router.post("/bulk")
def bulk(body: BulkIn):
    if not body.ids:
        return {"ok": True, "n": 0}
    ph = ",".join("?" * len(body.ids))
    with db.tx() as c:
        if body.action == "delete":
            c.execute(f"DELETE FROM leads WHERE id IN ({ph})", body.ids)
        elif body.action in ("archive", "unarchive"):
            c.execute(f"UPDATE leads SET archived=? WHERE id IN ({ph})", (int(body.action == "archive"), *body.ids))
        elif body.action == "status" and body.value:
            c.execute(f"UPDATE leads SET status=?, updated_at=datetime('now','localtime') WHERE id IN ({ph})", (body.value, *body.ids))
            for i in body.ids:
                _log(c, i, "status", f"Status → {body.value} (Mehrfachauswahl)")
        elif body.action == "star":
            c.execute(f"UPDATE leads SET starred=1-starred WHERE id IN ({ph})", body.ids)
    return {"ok": True, "n": len(body.ids)}


# ---------------------------------------------------------------- stats
@router.get("/stats")
def stats():
    today = date.today().isoformat()
    week = (date.today() - timedelta(days=7)).isoformat()
    with db.tx() as c:
        by = {}
        for r in c.execute("SELECT kind, status, COUNT(*) n FROM leads WHERE archived=0 GROUP BY kind, status"):
            by.setdefault(r["kind"], {})[r["status"]] = r["n"]
        due = c.execute("SELECT COUNT(*) FROM leads WHERE archived=0 AND next_action_date<=? AND status NOT IN ('gebucht','geliefert','retainer','kein_interesse')", (today,)).fetchone()[0]
        contacted_week = c.execute("SELECT COUNT(DISTINCT lead_id) FROM lead_activities WHERE type IN ('contact','call','visit','letter') AND ts>=?", (week,)).fetchone()[0]
        contacted_today = c.execute("SELECT COUNT(*) FROM lead_activities WHERE type IN ('contact','call','visit','letter') AND ts>=?", (today,)).fetchone()[0]
        dms_today = c.execute("SELECT COUNT(*) FROM lead_activities WHERE type='contact' AND channel='instagram' AND ts>=?", (today,)).fetchone()[0]
        contacted = c.execute("SELECT COUNT(*) FROM leads WHERE contact_count>0").fetchone()[0]
        replied = c.execute("SELECT COUNT(*) FROM leads WHERE replied_at IS NOT NULL").fetchone()[0]
        won = c.execute("SELECT COUNT(*), COALESCE(SUM(COALESCE(deal_value_final, deal_value_est)),0) FROM leads WHERE status IN ('gebucht','geliefert','retainer')").fetchone()
        pipe = c.execute("SELECT COALESCE(SUM(deal_value_est),0) FROM leads WHERE status IN ('geantwortet','gespraech','angebot') AND archived=0").fetchone()[0]
        total = c.execute("SELECT kind, COUNT(*) n, SUM(COALESCE(instagram,'')<>'') ig, SUM(COALESCE(email,'')<>'') em, SUM(COALESCE(phone,'')<>'') ph FROM leads WHERE archived=0 GROUP BY kind").fetchall()
        recent = c.execute("SELECT a.ts, a.type, a.channel, a.text, l.name, l.id, l.kind FROM lead_activities a JOIN leads l ON l.id=a.lead_id WHERE a.type<>'status' ORDER BY a.id DESC LIMIT 12").fetchall()
    return {"by_status": by, "due": due, "contacted_week": contacted_week, "contacted_today": contacted_today,
            "dms_today": dms_today, "contacted": contacted, "replied": replied,
            "reply_rate": round(100 * replied / contacted) if contacted else None, "won": won[0], "won_value": won[1],
            "pipeline_value": pipe, "totals": [dict(r) for r in total], "recent": [dict(r) for r in recent]}


# ---------------------------------------------------------------- finder jobs
class RestaurantFind(BaseModel):
    areas: list[str]
    types: list[str] = Field(default_factory=lambda: ["restaurant", "cafe"])
    enrich: bool = False


async def _find_restaurants(job, body: RestaurantFind):
    counts = {"new": 0, "updated": 0, "same": 0, "blocked": 0}
    job.update(total=len(body.areas))
    new_ids = []
    for i, a in enumerate(body.areas):
        job.update(done=i, message=f"OpenStreetMap: {a.split(':')[-1]} …")
        try:
            found = await osm.query(a, body.types)
        except Exception as e:  # one area failing must not kill the whole run
            counts.setdefault("errors", []).append(f"{a.split(':')[-1]}: {e}")
            continue
        with db.tx() as c:
            before = c.execute("SELECT COALESCE(MAX(id),0) FROM leads").fetchone()[0]
            for lead in found:
                counts[upsert(c, lead)] += 1
            new_ids += [r[0] for r in c.execute("SELECT id FROM leads WHERE id>? AND kind='restaurant'", (before,))]
    if counts.get("errors") and not (counts["new"] or counts["updated"] or counts["same"]):
        raise RuntimeError(" | ".join(counts["errors"]))
    job.update(done=len(body.areas), message=f"{counts['new']} neue Lokale")
    if body.enrich and new_ids:
        await _enrich(job, new_ids, counts)
    return counts


async def _enrich(job, ids: list[int], counts: dict | None = None):
    counts = counts if counts is not None else {}
    with db.tx() as c:
        rows = c.execute(f"SELECT id, name, website, email, instagram, phone FROM leads WHERE id IN ({','.join('?' * len(ids))}) "
                         "AND COALESCE(website,'')<>''", ids).fetchall()
    todo = [dict(r) for r in rows if not (r["email"] and r["instagram"])]
    job.update(done=0, total=len(todo), message="Websites durchsuchen (Impressum/Kontakt) …")
    found = {"email": 0, "instagram": 0, "phone": 0}
    sem = asyncio.Semaphore(4)

    async def one(r):
        async with sem:
            if job.cancelled:
                return
            res = await enrich.website(r["website"])
            upd = {k: res[k] for k in ("email", "instagram", "phone") if res.get(k) and not r.get(k)}
            with db.tx() as c:
                lead = _get(c, r["id"])
                extra = {**lead["extra"], "enriched": date.today().isoformat()}
                if res.get("emails"):
                    extra["emails"] = res["emails"]
                upd["extra"] = json.dumps(extra, ensure_ascii=False)
                upd["score"] = pipeline.score({**lead, **{k: v for k, v in upd.items() if k != "extra"}})
                c.execute(f"UPDATE leads SET {', '.join(f'{k}=?' for k in upd)} WHERE id=?", (*upd.values(), r["id"]))
            for k in found:
                if k in upd:
                    found[k] += 1
            job.update(done=job.done + 1, message=f"{r['name']}: " + (", ".join(k for k in found if k in upd) or "nichts Neues"))

    await asyncio.gather(*(one(r) for r in todo))
    counts.update({f"found_{k}": v for k, v in found.items()})
    return counts


@router.post("/find/restaurants")
async def find_restaurants(body: RestaurantFind):
    if not body.areas:
        raise HTTPException(400, "Mindestens ein Gebiet wählen")
    return {"job": jobs.start("Restaurants suchen", _find_restaurants, body)}


class EnrichIn(BaseModel):
    ids: list[int] = Field(default_factory=list)
    missing_only: bool = True
    limit: int = 150


@router.post("/enrich")
async def enrich_leads(body: EnrichIn):
    ids = body.ids
    if not ids:
        with db.tx() as c:
            ids = [r[0] for r in c.execute(
                "SELECT id FROM leads WHERE kind='restaurant' AND archived=0 AND COALESCE(website,'')<>'' AND "
                "(COALESCE(email,'')='' OR COALESCE(instagram,'')='') AND json_extract(extra,'$.enriched') IS NULL "
                "ORDER BY score DESC LIMIT ?", (body.limit,))]
    if not ids:
        raise HTTPException(400, "Keine Lokale mit Website ohne E-Mail/Instagram gefunden")
    return {"job": jobs.start(f"{len(ids)} Websites durchsuchen", _enrich, ids)}


class ArtistFind(BaseModel):
    sources: list[str] = Field(default_factory=lambda: ["musicbrainz", "wikidata"])
    regions: list[str] = Field(default_factory=lambda: ["wien", "moedling"])
    youtube_queries: list[str] = Field(default_factory=list)


async def _find_artists(job, body: ArtistFind):
    counts = {"new": 0, "updated": 0, "same": 0, "blocked": 0, "with_instagram": 0}
    errors = []
    for src in body.sources:
        try:
            if src == "musicbrainz":
                found = await artists.musicbrainz(job, body.regions)
            elif src == "wikidata":
                found = await artists.wikidata(job)
            elif src == "youtube":
                found = await artists.youtube_channels(job, body.youtube_queries or None)
            else:
                continue
        except Exception as e:
            errors.append(f"{src}: {e}")
            continue
        with db.tx() as c:
            for lead in found:
                counts[upsert(c, lead)] += 1
                counts["with_instagram"] += bool(lead.get("instagram"))
    counts["errors"] = errors
    return counts


@router.post("/find/artists")
async def find_artists(body: ArtistFind):
    return {"job": jobs.start("Artists suchen", _find_artists, body)}


# ---------------------------------------------------------------- import / export
class PasteIn(BaseModel):
    text: str
    kind: str = "artist"
    district: str | None = None
    category: str | None = None


@router.post("/import/paste")
def import_paste(body: PasteIn):
    """Paste IG handles / profile links (one per line or comma separated), optional 'Name - @handle'."""
    counts = {"new": 0, "updated": 0, "same": 0, "blocked": 0, "invalid": 0}
    with db.tx() as c:
        for line in re.split(r"[\n,;]+", body.text):
            line = line.strip()
            if not line:
                continue
            m = re.search(r"(instagram\.com/[A-Za-z0-9_.]+|@[A-Za-z0-9_.]+|^[A-Za-z0-9_.]+$)", line)
            ig = pipeline.normalize_instagram(m.group(1)) if m else None
            if not ig:
                counts["invalid"] += 1
                continue
            name = re.sub(r"(https?://)?(www\.)?instagram\.com/[A-Za-z0-9_.]+/?|@[A-Za-z0-9_.]+", "", line).strip(" -–:|") or ig
            counts[upsert(c, {"kind": body.kind, "name": name, "instagram": ig, "district": body.district,
                              "category": body.category, "source": "import", "source_id": f"ig:{ig}"})] += 1
    return counts


CSV_COLS = ["id", "kind", "name", "status", "score", "instagram", "email", "phone", "website", "address", "district",
            "category", "followers", "contact_count", "first_contact_at", "last_contact_at", "replied_at", "next_action_date",
            "deal_value_est", "deal_value_final", "notes", "source"]


@router.get("/export.csv")
def export_csv(kind: str = ""):
    with db.tx() as c:
        rows = c.execute("SELECT * FROM leads WHERE kind LIKE ? ORDER BY kind, score DESC", (kind or "%",)).fetchall()
    buf = io.StringIO()
    buf.write("﻿")  # Excel-friendly UTF-8
    w = csv.writer(buf, delimiter=";")
    w.writerow(CSV_COLS)
    for r in rows:
        w.writerow([r[k] for k in CSV_COLS])
    buf.seek(0)
    name = f"leads-{kind or 'alle'}-{date.today().isoformat()}.csv"
    return StreamingResponse(iter([buf.getvalue()]), media_type="text/csv",
                             headers={"Content-Disposition": f'attachment; filename="{name}"'})


@router.post("/import/csv")
async def import_csv(file: UploadFile = File(...), kind: str = "artist"):
    raw = (await file.read()).decode("utf-8-sig", errors="replace")
    dialect = csv.Sniffer().sniff(raw[:2000], delimiters=";,\t")
    counts = {"new": 0, "updated": 0, "same": 0, "blocked": 0}
    with db.tx() as c:
        for row in csv.DictReader(io.StringIO(raw), dialect=dialect):
            row = {k.strip().lower(): (v or "").strip() for k, v in row.items() if k}
            name = row.get("name") or row.get("username") or row.get("instagram")
            if not name:
                continue
            ig = pipeline.normalize_instagram(row.get("instagram") or row.get("username"))
            counts[upsert(c, {"kind": row.get("kind") or kind, "name": name, "instagram": ig, "email": row.get("email"),
                              "phone": pipeline.normalize_phone(row.get("phone") or row.get("telefon")),
                              "website": row.get("website"), "address": row.get("address") or row.get("adresse"),
                              "district": row.get("district") or row.get("bezirk"), "category": row.get("category"),
                              "notes": row.get("notes") or row.get("notizen"), "source": "csv",
                              "source_id": f"csv:{ig or row.get('email') or name}"})] += 1
    return counts


# ---------------------------------------------------------------- templates
@router.get("/templates")
def list_templates(kind: str = ""):
    _seed_templates()
    with db.tx() as c:
        rows = c.execute("SELECT * FROM lead_templates WHERE kind LIKE ? ORDER BY kind, sort, id", (kind or "%",)).fetchall()
    return [dict(r) for r in rows]


class TemplateIn(BaseModel):
    kind: str
    channel: str
    name: str
    subject: str = ""
    body: str


@router.post("/templates")
def add_template(t: TemplateIn):
    with db.tx() as c:
        c.execute("INSERT INTO lead_templates(kind, channel, name, subject, body, sort) VALUES(?,?,?,?,?,99)",
                  (t.kind, t.channel, t.name, t.subject, t.body))
    return {"ok": True}


@router.put("/templates/{tid}")
def update_template(tid: int, t: TemplateIn):
    with db.tx() as c:
        c.execute("UPDATE lead_templates SET kind=?, channel=?, name=?, subject=?, body=? WHERE id=?",
                  (t.kind, t.channel, t.name, t.subject, t.body, tid))
    return {"ok": True}


@router.delete("/templates/{tid}")
def delete_template(tid: int):
    with db.tx() as c:
        c.execute("DELETE FROM lead_templates WHERE id=?", (tid,))
    return {"ok": True}


@router.get("/leads/{lid}/render/{tid}")
def render_template(lid: int, tid: int):
    with db.tx() as c:
        lead = _get(c, lid)
        t = c.execute("SELECT * FROM lead_templates WHERE id=?", (tid,)).fetchone()
    if not t:
        raise HTTPException(404)
    body = pipeline.render(t["body"], lead, me())
    subject = pipeline.render(t["subject"] or "", lead, me())
    return {"subject": subject, "body": body, "channel": t["channel"],
            "open_placeholders": sorted(set(re.findall(r"\{(\w+)\}", body + subject))),
            "compliance": COMPLIANCE.get(t["channel"])}


# ---------------------------------------------------------------- walk-in route
def _dist(a, b) -> float:
    from math import asin, cos, radians, sin, sqrt
    la1, lo1, la2, lo2 = map(radians, (a[0], a[1], b[0], b[1]))
    h = sin((la2 - la1) / 2) ** 2 + cos(la1) * cos(la2) * sin((lo2 - lo1) / 2) ** 2
    return 12742 * asin(sqrt(h))  # km


@router.get("/route")
def walk_route(district: str = "", stops: int = 8, status: str = "neu", min_score: int = 0):
    """Best not-yet-contacted venues in one area, ordered as a short walking route (nearest neighbour)."""
    stops = max(2, min(stops, 10))  # Google Maps allows 10 points in a link
    sts = status.split(",")
    with db.tx() as c:
        rows = c.execute(
            f"SELECT * FROM leads WHERE kind='restaurant' AND archived=0 AND lat IS NOT NULL AND district LIKE ? "
            f"AND status IN ({','.join('?' * len(sts))}) AND score>=? AND COALESCE(json_extract(flags,'$.do_not_contact'),0)=0 "
            f"ORDER BY starred DESC, score DESC LIMIT 60", (district or "%", *sts, min_score)).fetchall()
    cands = [_row(r) for r in rows]
    if not cands:
        return {"stops": [], "km": 0, "maps_url": None}
    # take the best lead as anchor, then the best-scored ones close to it
    anchor = cands[0]
    near = sorted(cands, key=lambda l: _dist((anchor["lat"], anchor["lon"]), (l["lat"], l["lon"])) - l["score"] / 100)[:stops]
    route, rest = [near[0]], near[1:]
    while rest:
        last = route[-1]
        nxt = min(rest, key=lambda l: _dist((last["lat"], last["lon"]), (l["lat"], l["lon"])))
        route.append(nxt)
        rest.remove(nxt)
    km = sum(_dist((a["lat"], a["lon"]), (b["lat"], b["lon"])) for a, b in zip(route, route[1:]))
    pts = [f"{l['lat']},{l['lon']}" for l in route]
    from urllib.parse import quote
    url = (f"https://www.google.com/maps/dir/?api=1&travelmode=walking&origin={pts[0]}&destination={pts[-1]}"
           + (f"&waypoints={quote('|'.join(pts[1:-1]))}" if len(pts) > 2 else ""))
    return {"stops": [{k: l[k] for k in ("id", "name", "address", "category", "score", "phone", "instagram", "status")} for l in route],
            "km": round(km, 1), "minutes": round(km / 4.5 * 60 + len(route) * 8), "maps_url": url}


@router.post("/rescore")
def rescore():
    """Recompute every score (after changing the rules in pipeline.py)."""
    with db.tx() as c:
        rows = [_row(r) for r in c.execute("SELECT * FROM leads")]
        for l in rows:
            c.execute("UPDATE leads SET score=? WHERE id=?", (pipeline.score(l), l["id"]))
    return {"updated": len(rows)}
