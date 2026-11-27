"""/api/shoots/* - shoot planning: shot list, gear, deliverables, weather + golden hour, call sheet."""
from __future__ import annotations

import json
from datetime import date, timedelta
from html import escape

from fastapi import APIRouter, HTTPException
from fastapi.responses import HTMLResponse
from pydantic import BaseModel, Field

from ... import cache, db
from ...config import settings
from ...net import client
from .sun import sun_times

router = APIRouter()
JSONF = ("shots", "gear", "deliverables")

TEMPLATES = {
    "musikvideo": {
        "label": "Musikvideo",
        "shots": ["Performance – Totale (ganzer Song durch)", "Performance – Halbnah (ganzer Song)", "Performance – Close-ups Hook",
                  "B-Roll Location / Atmosphäre", "Detail-Shots (Hände, Schmuck, Outfit)", "Gimbal-Walk / Movement-Shot",
                  "Slow-Motion (60/120 fps) für Hook", "Vertikale 9:16-Clips für Reels", "Behind-the-Scenes für eigenes IG"],
        "gear": ["Kamera + 2 Akkus geladen", "Speicherkarten leer & formatiert", "Objektive (Weitwinkel, 50mm, Tele)", "Gimbal",
                 "Stativ", "Licht (LED + Akkus)", "ND-Filter", "Bluetooth-Box für Playback", "Song-Datei offline am Handy",
                 "Gaffa-Tape, Kabel, Ersatzakku Handy", "Drehgenehmigung (falls öffentlicher Ort)"],
        "deliverables": [("Musikvideo 16:9 (YouTube) – 1. Schnitt", 7), ("Korrekturschleife", 10), ("Finales Video + Thumbnail-Still", 14),
                         ("3 Reels 9:16 (Hook-Snippets)", 14)],
    },
    "food": {
        "label": "Food / Restaurant",
        "shots": ["Signature-Gericht – Hero-Shot (45°)", "Top-Down Flatlay (3–4 Gerichte)", "Detail: Sauce / Anschnitt / Dampf",
                  "Koch in Action (Pfanne, Flamme, Anrichten)", "Interior – Totale bei Tageslicht", "Bar / Getränke", "Team-Portrait",
                  "Außenansicht / Schild", "Reel: Gericht wird serviert (9:16)", "Reel: Hände + Zutaten Slow-Motion"],
        "gear": ["Kamera + Akkus", "Makro / 50mm / 24-70mm", "Stativ + Top-Down-Arm", "Reflektor / Diffusor", "LED-Licht + Softbox",
                 "Pinsel, Öl-Spray, Pinzette (Food-Styling)", "Hintergründe / Teller-Auswahl abgesprochen", "Speicherkarten"],
        "deliverables": [("Auswahl / Kontaktbogen an Kunden", 3), ("20 bearbeitete Fotos (Web + Druck)", 7), ("3 Reels 9:16", 10),
                         ("Formate für Lieferando/Wolt zugeschnitten", 10)],
    },
    "imagefilm": {
        "label": "Imagefilm",
        "shots": ["Establishing Shot außen", "Interior Totale + Kamerafahrt", "Inhaber-Interview (2 Kameras)", "Team bei der Arbeit",
                  "Gäste-Atmosphäre (mit Einverständnis)", "Details & Produkte", "Abschluss-Shot / Logo-Moment"],
        "gear": ["2 Kameras", "Funkstrecke / Lavalier-Mikro", "Interview-Licht (Key + Fill)", "Gimbal", "Slider", "Einverständniserklärungen ausgedruckt"],
        "deliverables": [("Imagefilm 60–90 s – 1. Schnitt", 10), ("Korrekturschleife", 14), ("Final + 2 Kurzversionen (15 s / 30 s)", 18)],
    },
    "reels": {
        "label": "Reel-Paket",
        "shots": ["Hook-Shot (erste 1–2 Sek. zählen!)", "Trend-Format nachstellen", "Behind-the-Scenes", "Text-on-Screen Shot", "Loop-Ende"],
        "gear": ["Kamera oder Handy + Gimbal", "Licht", "Mikro", "Akkus"],
        "deliverables": [("5 Reels 9:16 inkl. Untertitel", 7)],
    },
}


def _row(r) -> dict:
    return db.row_to_dict(r, JSONF)


class ShootIn(BaseModel):
    title: str = ""
    type: str = "musikvideo"
    lead_id: int | None = None
    document_id: int | None = None
    date: str | None = None
    call_time: str = ""
    end_time: str = ""
    location: str = ""
    contact: str = ""
    crew: str = ""
    status: str = "geplant"
    shots: list = Field(default_factory=list)
    gear: list = Field(default_factory=list)
    deliverables: list = Field(default_factory=list)
    notes: str = ""


@router.get("/meta")
def meta():
    return {"types": {k: v["label"] for k, v in TEMPLATES.items()}}


@router.get("/shoots")
def list_shoots(upcoming: bool = False):
    with db.tx() as c:
        if upcoming:
            rows = c.execute("SELECT * FROM shoots WHERE status IN ('geplant') AND date>=? ORDER BY date, call_time",
                             (date.today().isoformat(),)).fetchall()
        else:
            rows = c.execute("SELECT * FROM shoots ORDER BY CASE WHEN status IN ('geliefert','abgesagt') THEN 1 ELSE 0 END, date").fetchall()
    out = [_row(r) for r in rows]
    for s in out:
        open_d = [d for d in s["deliverables"] if not d.get("done")]
        s["next_deadline"] = min((d.get("due") or "9999" for d in open_d), default=None)
    return out


@router.post("/shoots")
def create_shoot(s: ShootIn):
    t = TEMPLATES.get(s.type, TEMPLATES["musikvideo"])
    base = date.fromisoformat(s.date) if s.date else date.today() + timedelta(days=7)
    if s.lead_id:
        with db.tx() as c:
            lead = c.execute("SELECT name, kind, address, lat, lon, phone, instagram FROM leads WHERE id=?", (s.lead_id,)).fetchone()
        if lead:
            s.title = s.title or f"{'Musikvideo' if lead['kind'] == 'artist' else 'Content-Dreh'} – {lead['name']}"
            s.type = s.type if s.type != "musikvideo" or lead["kind"] == "artist" else "food"
            t = TEMPLATES[s.type]
            s.location = s.location or (lead["address"] or "")
            s.contact = s.contact or " · ".join(x for x in [lead["name"], lead["phone"], lead["instagram"] and "@" + lead["instagram"]] if x)
    shots = s.shots or [{"text": x, "done": False} for x in t["shots"]]
    gear = s.gear or [{"text": x, "done": False} for x in t["gear"]]
    deliv = s.deliverables or [{"text": x, "due": (base + timedelta(days=d)).isoformat(), "done": False} for x, d in t["deliverables"]]
    with db.tx() as c:
        cur = c.execute("INSERT INTO shoots(title, type, lead_id, document_id, date, call_time, end_time, location, contact, crew, status, "
                        "shots, gear, deliverables, notes) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                        (s.title or "Neuer Dreh", s.type, s.lead_id, s.document_id, base.isoformat(), s.call_time, s.end_time,
                         s.location, s.contact, s.crew, s.status, json.dumps(shots, ensure_ascii=False), json.dumps(gear, ensure_ascii=False),
                         json.dumps(deliv, ensure_ascii=False), s.notes))
        if s.lead_id and c.execute("SELECT 1 FROM sqlite_master WHERE name='lead_activities'").fetchone():
            c.execute("INSERT INTO lead_activities(lead_id, type, channel, text) VALUES(?,?,?,?)",
                      (s.lead_id, "note", "shoot", f"🎬 Dreh geplant für {base.isoformat()}"))
    return get_shoot(cur.lastrowid)


@router.get("/shoots/{sid}")
def get_shoot(sid: int):
    with db.tx() as c:
        r = c.execute("SELECT * FROM shoots WHERE id=?", (sid,)).fetchone()
    if not r:
        raise HTTPException(404)
    return _row(r)


@router.put("/shoots/{sid}")
async def update_shoot(sid: int, s: ShootIn):
    old = get_shoot(sid)
    lat, lon = old["lat"], old["lon"]
    if s.location and s.location != old["location"]:
        lat, lon = await geocode(s.location)
    with db.tx() as c:
        c.execute("UPDATE shoots SET title=?, type=?, date=?, call_time=?, end_time=?, location=?, lat=?, lon=?, contact=?, crew=?, status=?, "
                  "shots=?, gear=?, deliverables=?, notes=? WHERE id=?",
                  (s.title, s.type, s.date, s.call_time, s.end_time, s.location, lat, lon, s.contact, s.crew, s.status,
                   json.dumps(s.shots, ensure_ascii=False), json.dumps(s.gear, ensure_ascii=False),
                   json.dumps(s.deliverables, ensure_ascii=False), s.notes, sid))
        if s.status != old["status"] and old["lead_id"] and c.execute("SELECT 1 FROM sqlite_master WHERE name='leads'").fetchone():
            c.execute("INSERT INTO lead_activities(lead_id, type, channel, text) VALUES(?,?,?,?)",
                      (old["lead_id"], "note", "shoot", f"🎬 Dreh „{s.title}“ → {s.status}"))
            if s.status == "geliefert":
                # the best moment to ask for a testimonial + referral
                nxt = (date.today() + timedelta(days=3)).isoformat()
                row = c.execute("SELECT kind, flags FROM leads WHERE id=?", (old["lead_id"],)).fetchone()
                if row:
                    flags = json.loads(row["flags"] or "{}")
                    flags["delivered"] = True
                    c.execute("UPDATE leads SET status=CASE WHEN status='retainer' THEN status ELSE ? END, flags=?, next_action_date=? WHERE id=?",
                              ("geliefert" if row["kind"] == "artist" else "gebucht", json.dumps(flags), nxt, old["lead_id"]))
                    c.execute("INSERT INTO lead_activities(lead_id, type, channel, text) VALUES(?,?,?,?)",
                              (old["lead_id"], "note", "shoot", "⭐ In 3 Tagen: nach Feedback, Testimonial/Google-Bewertung und Empfehlung fragen"))
    return get_shoot(sid)


@router.delete("/shoots/{sid}")
def delete_shoot(sid: int):
    with db.tx() as c:
        c.execute("DELETE FROM shoots WHERE id=?", (sid,))
    return {"ok": True}


async def geocode(q: str) -> tuple[float | None, float | None]:
    key = cache.make_key("nominatim", "search", {"q": q})
    hit = cache.get(key)
    if hit is None:
        try:
            r = await client().get("https://nominatim.openstreetmap.org/search",
                                   params={"q": q, "format": "json", "limit": 1, "countrycodes": "at"},
                                   headers={"User-Agent": "BeatDashboard/1.0 (local tool)"}, timeout=10)
            data = r.json() if r.status_code == 200 else []
            hit = [float(data[0]["lat"]), float(data[0]["lon"])] if data else [None, None]
            cache.put(key, "nominatim", hit, 180 * 24 * 3600)
        except Exception:
            return None, None
    return hit[0], hit[1]


WMO = {0: "☀️ klar", 1: "🌤️ überwiegend klar", 2: "⛅ teils bewölkt", 3: "☁️ bedeckt", 45: "🌫️ Nebel", 48: "🌫️ Nebel",
       51: "🌦️ Niesel", 53: "🌦️ Niesel", 55: "🌦️ Niesel", 61: "🌧️ Regen", 63: "🌧️ Regen", 65: "🌧️ starker Regen",
       71: "🌨️ Schnee", 73: "🌨️ Schnee", 75: "🌨️ starker Schnee", 80: "🌦️ Schauer", 81: "🌦️ Schauer", 82: "⛈️ heftige Schauer",
       95: "⛈️ Gewitter", 96: "⛈️ Gewitter", 99: "⛈️ Gewitter"}


@router.get("/shoots/{sid}/conditions")
async def conditions(sid: int):
    s = get_shoot(sid)
    lat, lon = s["lat"] or 48.0857, s["lon"] or 16.2850  # default: Mödling
    d = date.fromisoformat(s["date"]) if s["date"] else date.today()
    out = {"sun": sun_times(d, lat, lon), "weather": None}
    days = (d - date.today()).days
    if 0 <= days <= 15:
        key = cache.make_key("openmeteo", "daily", {"lat": round(lat, 2), "lon": round(lon, 2), "d": d.isoformat()})
        w = cache.get(key)
        if w is None:
            try:
                r = await client().get("https://api.open-meteo.com/v1/forecast", params={
                    "latitude": lat, "longitude": lon, "timezone": "Europe/Vienna", "start_date": d.isoformat(), "end_date": d.isoformat(),
                    "daily": "weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,wind_speed_10m_max"}, timeout=10)
                j = r.json()
                if r.status_code == 200 and "daily" in j:
                    dd = j["daily"]
                    w = {"text": WMO.get(dd["weather_code"][0], "–"), "tmax": dd["temperature_2m_max"][0], "tmin": dd["temperature_2m_min"][0],
                         "rain": dd["precipitation_probability_max"][0], "wind": dd["wind_speed_10m_max"][0]}
                    cache.put(key, "openmeteo", w, 3 * 3600)
            except Exception:
                w = None
        out["weather"] = w
    elif days > 15:
        out["weather_note"] = "Wettervorhersage gibt es ab 16 Tagen vor dem Dreh."
    return out


@router.get("/shoots/{sid}/callsheet", response_class=HTMLResponse)
async def callsheet(sid: int):
    s = get_shoot(sid)
    cond = await conditions(sid)
    e = lambda x: escape(str(x or "")).replace("\n", "<br>")
    sun, w = cond.get("sun") or {}, cond.get("weather")
    me = settings.get("MY_NAME") or settings.get("BIZ_NAME") or ""
    phone = settings.get("MY_PHONE") or ""
    maps = f"https://www.google.com/maps/search/?api=1&query={escape(s['location'] or '')}"
    lst = lambda items: "".join(f"<li>{'☑' if i.get('done') else '☐'} {e(i.get('text'))}</li>" for i in items)
    return HTMLResponse(f"""<!doctype html><html lang=de><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>Call Sheet – {e(s['title'])}</title><style>
body {{ font-family: -apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI", Inter, sans-serif; color: #1d1d1f; background: #f5f5f7; margin: 0; font-size: 14px; }}
.page {{ max-width: 760px; margin: 20px auto; background: #fff; border-radius: 18px; padding: 28px; box-shadow: 0 10px 40px rgba(0,0,0,.06); }}
h1 {{ font-size: 26px; letter-spacing: -.03em; margin: 0; }} .muted {{ color: #6e6e73; }} h3 {{ margin: 22px 0 8px; font-size: 15px; }}
.grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 10px; margin-top: 16px; }}
.box {{ background: #f5f5f7; border-radius: 12px; padding: 10px 12px; }} .box b {{ display: block; font-size: 17px; }} .box span {{ font-size: 12px; color: #6e6e73; }}
ul {{ padding-left: 0; list-style: none; }} li {{ padding: 5px 0; border-bottom: 1px solid #f0f0f2; }}
a {{ color: #007aff; }} button {{ background: #007aff; color: #fff; border: 0; border-radius: 99px; padding: 8px 16px; font: inherit; }}
@media print {{ body {{ background: #fff; }} .page {{ box-shadow: none; margin: 0; }} button {{ display: none; }} }}
</style></head><body><div class=page>
<button onclick=print() style="float:right">Drucken</button>
<span class=muted>CALL SHEET</span><h1>{e(s['title'])}</h1>
<div class=grid>
<div class=box><span>Datum</span><b>{e(s['date'])}</b></div>
<div class=box><span>Call Time</span><b>{e(s['call_time'] or '–')}</b>{('<span>bis ' + e(s['end_time']) + '</span>') if s['end_time'] else ''}</div>
<div class=box><span>Golden Hour (Abend)</span><b>{e(sun.get('golden_evening', '–'))}</b><span>Sonnenuntergang {e(sun.get('sunset', '–'))}</span></div>
<div class=box><span>Wetter</span><b>{e(w['text']) if w else '–'}</b>{('<span>' + str(w['tmin']) + '–' + str(w['tmax']) + ' °C · Regen ' + str(w['rain']) + ' %</span>') if w else ''}</div>
</div>
<h3>📍 Location</h3><p>{e(s['location'] or '–')} {f'· <a href="{maps}">Karte</a>' if s['location'] else ''}</p>
<h3>👥 Kontakte</h3><p>{e(s['contact'] or '–')}<br>Regie/Kamera: {e(me)} {e(phone)}{('<br>Crew: ' + e(s['crew'])) if s['crew'] else ''}</p>
<h3>🎬 Shotlist</h3><ul>{lst(s['shots'])}</ul>
<h3>🎒 Equipment</h3><ul>{lst(s['gear'])}</ul>
<h3>📦 Lieferung</h3><ul>{''.join(f"<li>{'☑' if d.get('done') else '☐'} {e(d.get('text'))} <span class=muted>– bis {e(d.get('due'))}</span></li>" for d in s['deliverables'])}</ul>
{('<h3>📝 Notizen</h3><p>' + e(s['notes']) + '</p>') if s['notes'] else ''}
</div></body></html>""")
