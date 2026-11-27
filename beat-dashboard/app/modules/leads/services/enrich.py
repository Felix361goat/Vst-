"""Find email / Instagram / phone on a business website (home + Impressum/Kontakt page).

Austrian websites must publish an Impressum (§5 ECG / §25 MedienG) - it nearly always contains an email.
Polite: max 3 pages per site, short timeouts, identifies itself.
"""
from __future__ import annotations

import re
from html import unescape
from urllib.parse import urljoin, urlparse

from ....net import client
from .pipeline import normalize_instagram, normalize_phone

EMAIL = re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}")
OBFUSCATED = re.compile(r"([A-Za-z0-9._%+-]+)\s*(?:\(at\)|\[at\]| at |\{at\}|\(ät\))\s*([A-Za-z0-9.-]+)\s*(?:\(dot\)|\[dot\]|\.| dot )\s*([A-Za-z]{2,})", re.I)
IG = re.compile(r"instagram\.com/([A-Za-z0-9_.]{2,30})", re.I)
TEL = re.compile(r"href=[\"']tel:([^\"']+)", re.I)
LINK = re.compile(r"<a[^>]+href=[\"']([^\"'#]+)[\"'][^>]*>(.*?)</a>", re.I | re.S)
BAD_MAIL = ("example.", "sentry", "wixpress", "domain.", "@2x", ".png", ".jpg", ".webp", ".gif", "u003e", "email@", "name@")
KEYWORDS = ("impressum", "kontakt", "contact", "imprint", "about", "ueber-uns", "uber-uns")


def _emails(html: str) -> list[str]:
    text = unescape(html)
    found = EMAIL.findall(text) + ["@".join(m[:2]) + "." + m[2] for m in OBFUSCATED.findall(text)]
    out = []
    for e in found:
        e = e.strip(".").lower()
        if any(b in e for b in BAD_MAIL) or e in out:
            continue
        out.append(e)
    return out


def _rank_email(e: str, domain: str) -> int:
    local, _, host = e.partition("@")
    s = 0
    if domain and domain in host:
        s += 10
    if local in ("office", "info", "kontakt", "hallo", "hello", "reservierung", "restaurant", "marketing", "events"):
        s += 5
    if local.startswith(("noreply", "no-reply", "datenschutz", "privacy", "jobs", "bewerbung")):
        s -= 20
    return s


async def _get(url: str) -> str | None:
    try:
        r = await client().get(url, timeout=12)
        if r.status_code == 200 and "html" in r.headers.get("content-type", "html"):
            return r.text[:600_000]
    except Exception:
        return None
    return None


async def website(url: str) -> dict:
    if not url:
        return {}
    if not url.startswith("http"):
        url = "https://" + url
    domain = urlparse(url).netloc.replace("www.", "")
    home = await _get(url)
    if home is None:
        return {"error": "Website nicht erreichbar"}
    pages = [home]
    # follow up to 2 Impressum/Kontakt links on the same site
    seen = set()
    for href, label in LINK.findall(home):
        lab = re.sub(r"<[^>]+>", "", label).lower()
        h = href.lower()
        if any(k in h or k in lab for k in KEYWORDS):
            full = urljoin(url, href)
            if urlparse(full).netloc.replace("www.", "") == domain and full not in seen:
                seen.add(full)
        if len(seen) >= 2:
            break
    for link in seen:
        html = await _get(link)
        if html:
            pages.append(html)
    blob = "\n".join(pages)
    emails = sorted(_emails(blob), key=lambda e: -_rank_email(e, domain))
    igs = [normalize_instagram(m) for m in IG.findall(blob)]
    igs = [i for i in dict.fromkeys(igs) if i]
    phones = [p for p in dict.fromkeys(normalize_phone(t) for t in TEL.findall(blob)) if p]
    return {"email": emails[0] if emails else None, "emails": emails[:5], "instagram": igs[0] if igs else None,
            "phone": phones[0] if phones else None, "pages": 1 + len(seen)}
