"""/api/money/* - quotes, invoices (Austria, Kleinunternehmer-ready), simple ledger."""
from __future__ import annotations

import json
from datetime import date, timedelta
from html import escape

from fastapi import APIRouter, HTTPException
from fastapi.responses import HTMLResponse
from pydantic import BaseModel, Field

from ... import db
from ...config import settings

router = APIRouter()
KU_LIMIT = 55_000

PRESETS = [
    {"desc": "Musikvideo – halber Drehtag inkl. Schnitt & Color Grading", "price": 600},
    {"desc": "Musikvideo – ganzer Drehtag, mehrere Locations, Schnitt & Grading", "price": 1500},
    {"desc": "Visualizer / Performance-Video + 3 Reels (9:16)", "price": 450},
    {"desc": "Reel-Paket: 5 Kurzclips (9:16) aus einem Dreh", "price": 400},
    {"desc": "Food-Fotoshooting (bis 3 h, 20 bearbeitete Bilder)", "price": 550},
    {"desc": "Food- & Interior-Paket (Fotos + 3 Reels)", "price": 1000},
    {"desc": "Social-Content Retainer Basic (1 Dreh, 4 Reels, 10 Fotos / Monat)", "price": 800},
    {"desc": "Imagefilm 60–90 s inkl. 2 Kurzversionen", "price": 1800},
    {"desc": "Anfahrt / Kilometergeld", "price": 0.5},
    {"desc": "Zusätzliche Korrekturschleife", "price": 80},
]
RIGHTS = {
    "artist": "Nutzungsrechte: Der Auftraggeber erhält das zeitlich und örtlich unbeschränkte, nicht-exklusive Recht, "
              "das Video auf eigenen Kanälen (YouTube, Instagram, TikTok, Streaming-Dienste) zu veröffentlichen. "
              "Der Urheber darf Ausschnitte im eigenen Portfolio zeigen. Rohmaterial ist nicht Teil der Leistung.",
    "restaurant": "Nutzungsrechte: Der Auftraggeber erhält das nicht-exklusive Recht, Fotos und Videos zeitlich unbeschränkt "
                  "für eigene Werbezwecke (Website, Social Media, Lieferplattformen, Speisekarte, Print) zu nutzen. "
                  "Weitergabe an Dritte (z.B. Agenturen für Paid Ads) nach Absprache. Der Urheber darf die Arbeiten im Portfolio zeigen.",
}


def ku() -> bool:
    return (settings.get("KLEINUNTERNEHMER", "ja") or "ja").strip().lower() not in ("nein", "no", "0", "false")


def _total(items: list[dict]) -> float:
    return round(sum(float(i.get("qty") or 0) * float(i.get("price") or 0) for i in items), 2)


def _row(r) -> dict:
    return db.row_to_dict(r, ("items",))


def _next_number(c, type_: str, year: int) -> str:
    prefix = ("R" if type_ == "rechnung" else "A") + f"-{year}-"
    last = c.execute("SELECT number FROM documents WHERE number LIKE ? ORDER BY number DESC LIMIT 1", (prefix + "%",)).fetchone()
    n = int(last[0].rsplit("-", 1)[1]) + 1 if last else 1
    return f"{prefix}{n:03d}"


class DocIn(BaseModel):
    type: str = "angebot"
    lead_id: int | None = None
    client_name: str = ""
    client_address: str = ""
    client_email: str = ""
    title: str = ""
    items: list[dict] = Field(default_factory=list)
    intro: str = ""
    notes: str = ""
    rights: str = ""
    issue_date: str | None = None
    delivery: str = ""
    due_date: str | None = None
    valid_until: str | None = None


@router.get("/meta")
def meta():
    return {"presets": PRESETS, "rights": RIGHTS, "ku": ku(), "limit": KU_LIMIT,
            "biz": {k: settings.get(k) for k in ("BIZ_NAME", "BIZ_ADDRESS", "BIZ_EMAIL", "IBAN", "BIC", "UID")}}


@router.get("/documents")
def list_docs(type: str = "", status: str = ""):
    with db.tx() as c:
        rows = c.execute("SELECT * FROM documents WHERE type LIKE ? AND status LIKE ? ORDER BY id DESC",
                         (type or "%", status or "%")).fetchall()
    return [_row(r) for r in rows]


@router.post("/documents")
def create_doc(d: DocIn):
    today = date.today()
    if d.lead_id and not d.client_name:
        with db.tx() as c:
            lead = c.execute("SELECT * FROM leads WHERE id=?", (d.lead_id,)).fetchone()
        if lead:
            d.client_name, d.client_address, d.client_email = lead["name"], lead["address"] or "", lead["email"] or ""
            if not d.rights:
                d.rights = RIGHTS.get(lead["kind"], RIGHTS["restaurant"])
    days = int(settings.get("PAYMENT_DAYS", "14") or 14)
    with db.tx() as c:
        cur = c.execute(
            "INSERT INTO documents(type, lead_id, client_name, client_address, client_email, title, items, intro, notes, rights, "
            "issue_date, delivery, due_date, valid_until, total) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (d.type, d.lead_id, d.client_name, d.client_address, d.client_email, d.title, json.dumps(d.items, ensure_ascii=False),
             d.intro, d.notes, d.rights, d.issue_date or today.isoformat(), d.delivery,
             d.due_date or (today + timedelta(days=days)).isoformat(), d.valid_until or (today + timedelta(days=30)).isoformat(),
             _total(d.items)))
    return get_doc(cur.lastrowid)


@router.get("/documents/{did}")
def get_doc(did: int):
    with db.tx() as c:
        r = c.execute("SELECT * FROM documents WHERE id=?", (did,)).fetchone()
    if not r:
        raise HTTPException(404)
    return _row(r)


@router.put("/documents/{did}")
def update_doc(did: int, d: DocIn):
    doc = get_doc(did)
    if doc["number"] and doc["type"] == "rechnung" and doc["status"] != "entwurf":
        raise HTTPException(409, "Versendete Rechnungen nicht mehr ändern – stattdessen stornieren und neu ausstellen.")
    with db.tx() as c:
        c.execute("UPDATE documents SET client_name=?, client_address=?, client_email=?, title=?, items=?, intro=?, notes=?, rights=?, "
                  "issue_date=?, delivery=?, due_date=?, valid_until=?, total=? WHERE id=?",
                  (d.client_name, d.client_address, d.client_email, d.title, json.dumps(d.items, ensure_ascii=False), d.intro,
                   d.notes, d.rights, d.issue_date, d.delivery, d.due_date, d.valid_until, _total(d.items), did))
    return get_doc(did)


@router.delete("/documents/{did}")
def delete_doc(did: int):
    doc = get_doc(did)
    if doc["type"] == "rechnung" and doc["number"]:
        raise HTTPException(409, "Rechnungen mit Nummer dürfen nicht gelöscht werden (lückenlose Nummerierung) – bitte stornieren.")
    with db.tx() as c:
        c.execute("DELETE FROM documents WHERE id=?", (did,))
    return {"ok": True}


class StatusIn(BaseModel):
    status: str


def _lead_update(c, lead_id, **vals):
    if not lead_id or not c.execute("SELECT 1 FROM sqlite_master WHERE name='leads'").fetchone():
        return
    sets = ", ".join(f"{k}=?" for k in vals)
    c.execute(f"UPDATE leads SET {sets}, updated_at=datetime('now','localtime') WHERE id=?", (*vals.values(), lead_id))


def _lead_log(c, lead_id, text):
    if lead_id and c.execute("SELECT 1 FROM sqlite_master WHERE name='lead_activities'").fetchone():
        c.execute("INSERT INTO lead_activities(lead_id, type, channel, text) VALUES(?,?,?,?)", (lead_id, "note", "money", text))


@router.post("/documents/{did}/status")
def set_status(did: int, body: StatusIn):
    doc = get_doc(did)
    today = date.today().isoformat()
    with db.tx() as c:
        number = doc["number"]
        if body.status in ("gesendet", "bezahlt", "angenommen") and not number:
            number = _next_number(c, doc["type"], date.today().year)
        c.execute("UPDATE documents SET status=?, number=?, paid_at=? WHERE id=?",
                  (body.status, number, today if body.status == "bezahlt" else doc["paid_at"], did))
        label = f"{'Angebot' if doc['type'] == 'angebot' else 'Rechnung'} {number or ''} → {body.status} ({doc['total']:.2f} €)"
        _lead_log(c, doc["lead_id"], label)
        if doc["type"] == "angebot" and body.status == "gesendet":
            _lead_update(c, doc["lead_id"], status="angebot", deal_value_est=doc["total"])
        if doc["type"] == "angebot" and body.status == "angenommen":
            _lead_update(c, doc["lead_id"], status="gebucht", deal_value_final=doc["total"], next_action_date=None)
        if doc["type"] == "rechnung" and body.status == "bezahlt":
            c.execute("DELETE FROM ledger WHERE document_id=?", (did,))
            c.execute("INSERT INTO ledger(date, kind, category, description, amount, document_id) VALUES(?,?,?,?,?,?)",
                      (today, "einnahme", "Video/Foto", f"Rechnung {number} – {doc['client_name']}", doc["total"], did))
            if doc["lead_id"] and c.execute("SELECT 1 FROM sqlite_master WHERE name='leads'").fetchone():
                c.execute("UPDATE leads SET deal_value_final=COALESCE(deal_value_final, ?), status=CASE WHEN status IN "
                          "('geliefert','retainer') THEN status ELSE 'gebucht' END WHERE id=?", (doc["total"], doc["lead_id"]))
        if body.status == "storniert":
            c.execute("DELETE FROM ledger WHERE document_id=?", (did,))
    return get_doc(did)


@router.post("/documents/{did}/to-invoice")
def to_invoice(did: int):
    q = get_doc(did)
    d = DocIn(type="rechnung", lead_id=q["lead_id"], client_name=q["client_name"], client_address=q["client_address"],
              client_email=q["client_email"], title=q["title"], items=q["items"], notes="", rights=q["rights"],
              delivery=q["delivery"])
    if q["status"] != "angenommen":
        set_status(did, StatusIn(status="angenommen"))  # also books the lead
    inv = create_doc(d)
    with db.tx() as c:
        c.execute("UPDATE documents SET source_id=? WHERE id=?", (did, inv["id"]))
    return get_doc(inv["id"])


def _money(v: float) -> str:
    return f"{v:,.2f} €".replace(",", "X").replace(".", ",").replace("X", ".")


def _de(iso: str | None) -> str:
    try:
        y, m, d = (iso or "").split("-")
        return f"{int(d)}.{int(m)}.{y}"
    except ValueError:
        return iso or ""


def _epc_svg(doc: dict) -> str:
    iban = (settings.get("IBAN") or "").replace(" ", "")
    if not iban or doc["type"] != "rechnung" or doc["total"] <= 0:
        return ""
    try:
        import segno
    except ImportError:
        return ""
    name = (settings.get("BIZ_NAME") or settings.get("MY_NAME") or "Empfänger")[:70]
    payload = "\n".join(["BCD", "002", "1", "SCT", (settings.get("BIC") or "").strip(), name, iban,
                         f"EUR{doc['total']:.2f}", "", "", (doc["number"] or "Rechnung")[:140]])
    return segno.make(payload, error="m").svg_inline(scale=3, dark="#1d1d1f")


@router.get("/documents/{did}/print", response_class=HTMLResponse)
def print_doc(did: int):
    d = get_doc(did)
    e = lambda s: escape(str(s or "")).replace("\n", "<br>")
    is_inv = d["type"] == "rechnung"
    title = ("Rechnung" if is_inv else "Angebot") + (f" {d['number']}" if d["number"] else " (Entwurf)")
    rows = "".join(
        f"<tr><td>{i + 1}</td><td>{e(it.get('desc'))}</td><td class=r>{float(it.get('qty') or 0):g}</td>"
        f"<td class=r>{_money(float(it.get('price') or 0))}</td><td class=r>{_money(float(it.get('qty') or 0) * float(it.get('price') or 0))}</td></tr>"
        for i, it in enumerate(d["items"]))
    tax = ("Umsatzsteuerfrei aufgrund der Kleinunternehmerregelung (§ 6 Abs. 1 Z 27 UStG)." if ku()
           else "Alle Beträge inkl. 20 % USt.: " + _money(d["total"] / 1.2 * 0.2) + " enthalten.")
    biz = {k: settings.get(k) for k in ("BIZ_NAME", "BIZ_ADDRESS", "BIZ_EMAIL", "IBAN", "BIC", "UID", "MY_PHONE")}
    qr = _epc_svg(d)
    pay = (f"<div class=pay><div><b>Zahlbar bis {_de(d['due_date'])}</b> ohne Abzug auf<br>IBAN {e(biz['IBAN'] or '[IBAN in Einstellungen]')}"
           f"{'<br>BIC ' + e(biz['BIC']) if biz['BIC'] else ''}<br>Verwendungszweck: {e(d['number'] or '')}</div>"
           f"{'<div class=qr>' + qr + '<small>Mit Banking-App scannen</small></div>' if qr else ''}</div>") if is_inv else \
          f"<p>Dieses Angebot ist gültig bis <b>{_de(d['valid_until'])}</b>. Bei Annahme werden 50 % Anzahlung vor dem Drehtag fällig.</p>"
    html = f"""<!doctype html><html lang=de><head><meta charset=utf-8><title>{e(title)} – {e(d['client_name'])}</title>
<style>
@page {{ size: A4; margin: 18mm 18mm 20mm; }}
* {{ box-sizing: border-box; }}
body {{ font-family: -apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI", Inter, Roboto, sans-serif; color: #1d1d1f; font-size: 10.5pt; line-height: 1.5; margin: 0; background: #f5f5f7; }}
.page {{ background: #fff; max-width: 210mm; margin: 24px auto; padding: 18mm; box-shadow: 0 10px 40px rgba(0,0,0,.08); border-radius: 6px; }}
header {{ display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 14mm; }}
.brand {{ font-size: 16pt; font-weight: 700; letter-spacing: -.02em; }} .muted {{ color: #6e6e73; }} small {{ color: #86868b; }}
h1 {{ font-size: 20pt; letter-spacing: -.03em; margin: 0 0 2mm; }}
.meta {{ display: grid; grid-template-columns: auto auto; gap: 1mm 6mm; font-size: 9.5pt; }}
table {{ width: 100%; border-collapse: collapse; margin: 8mm 0 4mm; }} th {{ text-align: left; font-weight: 500; color: #86868b; font-size: 9pt; border-bottom: 1px solid #d2d2d7; padding: 2mm 1.5mm; }}
td {{ padding: 2.5mm 1.5mm; border-bottom: 1px solid #f0f0f2; vertical-align: top; }} .r {{ text-align: right; white-space: nowrap; }}
.total td {{ border: 0; font-size: 13pt; font-weight: 700; padding-top: 4mm; }}
.pay {{ display: flex; justify-content: space-between; align-items: center; gap: 8mm; background: #f5f5f7; border-radius: 12px; padding: 5mm 6mm; margin-top: 6mm; }}
.qr {{ text-align: center; }} .qr svg {{ display: block; }}
footer {{ margin-top: 12mm; font-size: 8.5pt; color: #86868b; border-top: 1px solid #f0f0f2; padding-top: 3mm; }}
.bar {{ position: sticky; top: 0; background: rgba(245,245,247,.85); backdrop-filter: blur(20px); padding: 10px; text-align: center; }}
.bar button {{ background: #007aff; color: #fff; border: 0; border-radius: 99px; padding: 8px 18px; font: inherit; cursor: pointer; }}
@media print {{ body {{ background: #fff; }} .page {{ box-shadow: none; margin: 0; padding: 0; max-width: none; }} .bar {{ display: none; }} }}
</style></head><body>
<div class=bar><button onclick="print()">Drucken / als PDF speichern</button></div>
<div class=page>
<header><div><div class=brand>{e(biz['BIZ_NAME'] or '[Dein Name – Einstellungen]')}</div><div class=muted>{e(biz['BIZ_ADDRESS'])}<br>{e(biz['BIZ_EMAIL'])}{' · ' + e(biz['MY_PHONE']) if biz['MY_PHONE'] else ''}{'<br>UID ' + e(biz['UID']) if biz['UID'] else ''}</div></div>
<div class=meta><span class=muted>Datum</span><span>{_de(d['issue_date'])}</span>
<span class=muted>Leistung</span><span>{e(d['delivery']) if d['delivery'] else ('= Rechnungsdatum' if is_inv else 'nach Vereinbarung')}</span>
{'<span class=muted>Nummer</span><span>' + e(d['number']) + '</span>' if d['number'] else ''}</div></header>
<div style="margin-bottom:10mm"><small>An</small><br><b>{e(d['client_name'])}</b><br>{e(d['client_address'])}</div>
<h1>{e(title)}</h1><div class=muted>{e(d['title'])}</div>
{'<p>' + e(d['intro']) + '</p>' if d['intro'] else ''}
<table><tr><th>#</th><th>Leistung</th><th class=r>Menge</th><th class=r>Einzelpreis</th><th class=r>Gesamt</th></tr>{rows}
<tr class=total><td></td><td>Gesamtbetrag</td><td></td><td></td><td class=r>{_money(d['total'])}</td></tr></table>
<p class=muted>{tax}</p>
{pay}
{'<p style="margin-top:6mm">' + e(d['notes']) + '</p>' if d['notes'] else ''}
{'<p style="margin-top:6mm;font-size:9pt" class=muted>' + e(d['rights']) + '</p>' if d['rights'] else ''}
<footer>{e(biz['BIZ_NAME'])} · {e(biz['BIZ_ADDRESS'])}{' · IBAN ' + e(biz['IBAN']) if biz['IBAN'] else ''}</footer>
</div></body></html>"""
    return HTMLResponse(html)


# ---------------------------------------------------------------- ledger + overview
class LedgerIn(BaseModel):
    date: str
    kind: str = "ausgabe"
    category: str = ""
    description: str = ""
    amount: float


@router.get("/ledger")
def list_ledger(year: int | None = None):
    y = str(year or date.today().year)
    with db.tx() as c:
        rows = c.execute("SELECT * FROM ledger WHERE substr(date,1,4)=? ORDER BY date DESC, id DESC", (y,)).fetchall()
    return [dict(r) for r in rows]


@router.post("/ledger")
def add_ledger(e: LedgerIn):
    if e.kind not in ("einnahme", "ausgabe"):
        raise HTTPException(400, "kind = einnahme | ausgabe")
    with db.tx() as c:
        c.execute("INSERT INTO ledger(date, kind, category, description, amount) VALUES(?,?,?,?,?)",
                  (e.date, e.kind, e.category, e.description, abs(e.amount)))
    return {"ok": True}


@router.delete("/ledger/{lid}")
def del_ledger(lid: int):
    with db.tx() as c:
        c.execute("DELETE FROM ledger WHERE id=? AND document_id IS NULL", (lid,))
    return {"ok": True}


@router.get("/overview")
def overview(year: int | None = None):
    y = str(year or date.today().year)
    today = date.today().isoformat()
    with db.tx() as c:
        months = {f"{m:02d}": {"in": 0.0, "out": 0.0} for m in range(1, 13)}
        for r in c.execute("SELECT substr(date,6,2) m, kind, SUM(amount) s FROM ledger WHERE substr(date,1,4)=? GROUP BY m, kind", (y,)):
            months[r["m"]]["in" if r["kind"] == "einnahme" else "out"] = round(r["s"], 2)
        by_cat = [dict(r) for r in c.execute(
            "SELECT kind, COALESCE(NULLIF(category,''),'Sonstiges') category, SUM(amount) total FROM ledger WHERE substr(date,1,4)=? GROUP BY kind, category ORDER BY total DESC", (y,))]
        open_inv = [_row(r) for r in c.execute("SELECT * FROM documents WHERE type='rechnung' AND status='gesendet' ORDER BY due_date")]
        open_quotes = c.execute("SELECT COUNT(*), COALESCE(SUM(total),0) FROM documents WHERE type='angebot' AND status='gesendet'").fetchone()
    income = sum(m["in"] for m in months.values())
    expenses = sum(m["out"] for m in months.values())
    return {"year": int(y), "income": round(income, 2), "expenses": round(expenses, 2), "profit": round(income - expenses, 2),
            "months": months, "by_category": by_cat, "ku": ku(), "ku_limit": KU_LIMIT,
            "ku_pct": round(100 * income / KU_LIMIT, 1),
            "open_invoices": open_inv, "overdue": [d for d in open_inv if (d["due_date"] or "9") < today],
            "open_quotes": {"count": open_quotes[0], "total": open_quotes[1]}}
