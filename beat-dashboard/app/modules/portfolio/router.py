"""/api/portfolio/* + public pages /p/<niche> (one-page portfolio per target group)."""
from __future__ import annotations

import re
from html import escape

from fastapi import APIRouter, HTTPException
from fastapi.responses import HTMLResponse, Response
from pydantic import BaseModel

from ... import db
from ...config import settings

router = APIRouter()

NICHES = {
    "musikvideo": {
        "label": "Musikvideos & Reels", "headline": "Musikvideos, die deinen Release tragen.",
        "sub": "Musikvideos, Visualizer und Reels für Artists aus Wien & Umgebung – vom Konzept bis zum fertigen Clip.",
        "packages": [("Reel-Paket", 250, "4–6 Kurzclips 9:16 aus einem Dreh"), ("Visualizer + 3 Reels", 300, "Performance-Video + Snippets für deinen Release"),
                     ("Musikvideo", 600, "Konzept, Dreh, Schnitt & Color Grading")],
        "cta": "Schreib mir auf Instagram",
    },
    "food": {
        "label": "Food & Gastro Content", "headline": "Bilder, die hungrig machen.",
        "sub": "Food-Fotos, Reels und Imagefilme für Restaurants, Cafés und Bars in Wien & im Bezirk Mödling.",
        "packages": [("Food-Shooting", 350, "15–30 bearbeitete Fotos für Speisekarte, Google & Lieferplattformen"),
                     ("Social-Content monatlich", 600, "1 Dreh, 4 Reels & 10 Fotos pro Monat"), ("Imagefilm", 1000, "60–120 s plus Kurzversionen für Social Media")],
        "cta": "Unverbindlich anfragen",
    },
}


class ItemIn(BaseModel):
    niche: str
    title: str = ""
    client: str = ""
    url: str = ""
    image: str = ""
    result: str = ""
    sort: int = 0
    visible: int = 1


@router.get("/items")
def items(niche: str = ""):
    with db.tx() as c:
        rows = c.execute("SELECT * FROM portfolio_items WHERE niche LIKE ? ORDER BY sort, id", (niche or "%",)).fetchall()
    return {"items": [dict(r) for r in rows], "niches": {k: v["label"] for k, v in NICHES.items()}}


@router.post("/items")
def add_item(i: ItemIn):
    if i.niche not in NICHES:
        raise HTTPException(400, "Unbekannte Seite")
    with db.tx() as c:
        c.execute("INSERT INTO portfolio_items(niche, title, client, url, image, result, sort, visible) VALUES(?,?,?,?,?,?,?,?)",
                  (i.niche, i.title, i.client, i.url, i.image, i.result, i.sort, i.visible))
    return {"ok": True}


@router.put("/items/{iid}")
def update_item(iid: int, i: ItemIn):
    with db.tx() as c:
        c.execute("UPDATE portfolio_items SET niche=?, title=?, client=?, url=?, image=?, result=?, sort=?, visible=? WHERE id=?",
                  (i.niche, i.title, i.client, i.url, i.image, i.result, i.sort, i.visible, iid))
    return {"ok": True}


@router.delete("/items/{iid}")
def del_item(iid: int):
    with db.tx() as c:
        c.execute("DELETE FROM portfolio_items WHERE id=?", (iid,))
    return {"ok": True}


def _embed(url: str) -> str:
    """YouTube -> privacy-friendly embed; Instagram/TikTok/other -> link card."""
    m = re.search(r"(?:youtu\.be/|v=|shorts/|embed/)([A-Za-z0-9_-]{11})", url or "")
    if m:
        vid = m.group(1)
        return (f'<div class="media"><iframe src="https://www.youtube-nocookie.com/embed/{vid}" title="Video" loading="lazy" '
                f'allow="accelerometer; encrypted-media; picture-in-picture" allowfullscreen></iframe></div>')
    return ""


def render(niche: str) -> str:
    n = NICHES.get(niche)
    if not n:
        raise HTTPException(404)
    with db.tx() as c:
        rows = [dict(r) for r in c.execute("SELECT * FROM portfolio_items WHERE niche=? AND visible=1 ORDER BY sort, id", (niche,))]
    e = lambda s: escape(str(s or ""))
    name = settings.get("BIZ_NAME") or settings.get("MY_NAME") or "Dein Name"
    place = settings.get("MY_BEZIRK") or "Wien"
    ig = (settings.get("INSTAGRAM") or "").lstrip("@")
    mail = settings.get("BIZ_EMAIL") or ""
    contact_url = f"https://ig.me/m/{e(ig)}" if ig and niche == "musikvideo" else (f"mailto:{e(mail)}" if mail else (f"https://instagram.com/{e(ig)}" if ig else "#"))
    cards = []
    for r in rows:
        media = _embed(r["url"]) or (f'<div class="media"><img src="{e(r["image"])}" alt="{e(r["title"])}" loading="lazy"></div>' if r["image"] else "")
        link = f'<a href="{e(r["url"])}" target="_blank" rel="noopener">Ansehen ↗</a>' if r["url"] and not _embed(r["url"]) else ""
        cards.append(f'<article>{media}<div class="txt"><h3>{e(r["title"])}</h3><p class="muted">{e(r["client"])}</p>'
                     f'{"<p class=result>" + e(r["result"]) + "</p>" if r["result"] else ""}{link}</div></article>')
    pk = "".join(f'<div class="pk"><h3>{e(t)}</h3><p class="price">ab {p} €</p><p class="muted">{e(d)}</p></div>' for t, p, d in n["packages"])
    return f"""<!doctype html><html lang=de><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>{e(name)} – {e(n['label'])}</title><meta name=description content="{e(n['sub'])}">
<style>
:root {{ --t: #1d1d1f; --m: #6e6e73; --b: #007aff; }}
* {{ box-sizing: border-box; }} body {{ margin: 0; font-family: -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", Inter, sans-serif; color: var(--t); background: #fff; -webkit-font-smoothing: antialiased; }}
.wrap {{ max-width: 1080px; margin: 0 auto; padding: 0 22px; }}
nav {{ position: sticky; top: 0; background: rgba(255,255,255,.75); backdrop-filter: saturate(180%) blur(20px); -webkit-backdrop-filter: saturate(180%) blur(20px); border-bottom: 1px solid rgba(0,0,0,.06); z-index: 5; }}
nav .wrap {{ display: flex; justify-content: space-between; align-items: center; height: 52px; font-size: 14px; }}
.hero {{ text-align: center; padding: 90px 0 60px; }} .hero h1 {{ font-size: clamp(38px, 6vw, 64px); letter-spacing: -.035em; line-height: 1.05; margin: 0 0 16px; }}
.hero p {{ font-size: 20px; color: var(--m); max-width: 640px; margin: 0 auto 28px; line-height: 1.4; }}
.btn {{ display: inline-block; background: var(--b); color: #fff; padding: 12px 24px; border-radius: 99px; text-decoration: none; font-size: 17px; }}
.muted {{ color: var(--m); }} section {{ padding: 40px 0; }} h2 {{ font-size: 34px; letter-spacing: -.03em; text-align: center; margin: 0 0 30px; }}
.grid {{ display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 22px; }}
article {{ background: #f5f5f7; border-radius: 22px; overflow: hidden; }} .txt {{ padding: 18px 20px 22px; }} .txt h3 {{ margin: 0 0 4px; font-size: 19px; }}
.media {{ aspect-ratio: 16/9; background: #e8e8ed; }} .media iframe, .media img {{ width: 100%; height: 100%; border: 0; object-fit: cover; display: block; }}
.result {{ font-weight: 600; }} a {{ color: var(--b); text-decoration: none; }}
.pks {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 18px; }}
.pk {{ border: 1px solid rgba(0,0,0,.08); border-radius: 22px; padding: 26px; text-align: center; }} .pk h3 {{ margin: 0; }} .price {{ font-size: 28px; font-weight: 700; letter-spacing: -.02em; margin: 8px 0; }}
.cta {{ text-align: center; padding: 70px 0 90px; background: #f5f5f7; margin-top: 40px; }}
footer {{ text-align: center; font-size: 12px; color: var(--m); padding: 24px; }}
.empty {{ text-align: center; color: var(--m); }}
</style></head><body>
<nav><div class=wrap><b>{e(name)}</b><a href="{contact_url}">Kontakt</a></div></nav>
<div class="hero wrap"><p class=muted style="font-size:15px;margin-bottom:10px">{e(n['label'])} · {e(place)}</p><h1>{e(n['headline'])}</h1><p>{e(n['sub'])}</p><a class=btn href="{contact_url}">{e(n['cta'])}</a></div>
<section class=wrap><h2>Arbeiten</h2>{'<div class=grid>' + ''.join(cards) + '</div>' if cards else '<p class=empty>Hier erscheinen bald die ersten Projekte.</p>'}</section>
<section class=wrap><h2>Pakete</h2><div class=pks>{pk}</div><p class="muted" style="text-align:center;margin-top:16px">Individuelle Angebote nach kurzem Gespräch. Preise netto{', Kleinunternehmer – keine USt.' if (settings.get('KLEINUNTERNEHMER') or 'ja').lower() != 'nein' else ''}.</p></section>
<div class=cta><h2>Lass uns was machen.</h2><a class=btn href="{contact_url}">{e(n['cta'])}</a>{'<p class=muted style="margin-top:14px">' + e(mail) + '</p>' if mail else ''}</div>
<footer>© {e(name)} · {e(settings.get('BIZ_ADDRESS') or place)}</footer>
</body></html>"""


@router.get("/preview/{niche}", response_class=HTMLResponse)
def preview(niche: str):
    return HTMLResponse(render(niche))


@router.get("/export/{niche}")
def export(niche: str):
    return Response(render(niche), media_type="text/html",
                    headers={"Content-Disposition": f'attachment; filename="index-{niche}.html"'})
