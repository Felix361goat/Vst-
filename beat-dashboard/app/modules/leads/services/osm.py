"""Restaurants / cafés / bars from OpenStreetMap (Overpass API, free, ODbL)."""
from __future__ import annotations

from ....net import client

MIRRORS = [
    "https://overpass-api.de/api/interpreter",
    "https://maps.mail.ru/osm/tools/overpass/api/interpreter",
    "https://overpass.kumi.systems/api/interpreter",
]

WIEN_BEZIRKE = ["Innere Stadt", "Leopoldstadt", "Landstraße", "Wieden", "Margareten", "Mariahilf", "Neubau",
                "Josefstadt", "Alsergrund", "Favoriten", "Simmering", "Meidling", "Hietzing", "Penzing",
                "Rudolfsheim-Fünfhaus", "Ottakring", "Hernals", "Währing", "Döbling", "Brigittenau",
                "Floridsdorf", "Donaustadt", "Liesing"]
MOEDLING_GEMEINDEN = ["Mödling", "Perchtoldsdorf", "Brunn am Gebirge", "Maria Enzersdorf", "Wiener Neudorf",
                      "Vösendorf", "Guntramsdorf", "Gumpoldskirchen", "Hinterbrühl", "Laxenburg", "Biedermannsdorf",
                      "Breitenfurt bei Wien", "Kaltenleutgeben", "Gießhübl", "Achau", "Hennersdorf", "Gaaden",
                      "Laab im Walde", "Wienerwald"]
TYPES = {"restaurant": "Restaurant", "cafe": "Café", "bar": "Bar", "fast_food": "Fast Food / Imbiss",
         "pub": "Beisl / Pub", "ice_cream": "Eissalon", "bakery": "Bäckerei/Konditorei"}


def areas() -> list[dict]:
    out = [{"id": "wien", "label": "Ganz Wien (viele Treffer!)", "group": "Wien"}]
    out += [{"id": f"wien:{b}", "label": f"{i + 1}. {b}", "group": "Wien"} for i, b in enumerate(WIEN_BEZIRKE)]
    out += [{"id": "moedling", "label": "Ganzer Bezirk Mödling", "group": "Bezirk Mödling"}]
    out += [{"id": f"moedling:{g}", "label": g, "group": "Bezirk Mödling"} for g in MOEDLING_GEMEINDEN]
    return out


def _area_ql(area_id: str) -> str:
    if area_id == "wien":
        return 'area["name"="Wien"]["admin_level"="4"]->.a;'
    if area_id == "moedling":
        return 'area["name"="Bezirk Mödling"]["admin_level"="6"]->.a;'
    group, name = area_id.split(":", 1)
    if group == "wien" and name in WIEN_BEZIRKE:
        return (f'area["name"="Wien"]["admin_level"="4"]->.p;rel["name"="{name}"]["admin_level"="9"]'
                f'["boundary"="administrative"](area.p);map_to_area->.a;')
    if group == "moedling" and name in MOEDLING_GEMEINDEN:
        return (f'area["name"="Bezirk Mödling"]["admin_level"="6"]->.p;rel["name"="{name}"]["admin_level"="8"]'
                f'["boundary"="administrative"](area.p);map_to_area->.a;')
    raise ValueError(f"Unbekanntes Gebiet: {area_id}")


def _label(area_id: str) -> str:
    if area_id == "wien":
        return "Wien"
    if area_id == "moedling":
        return "Bezirk Mödling"
    group, name = area_id.split(":", 1)
    if group == "wien":
        return f"{WIEN_BEZIRKE.index(name) + 1}. {name}"
    return name


async def query(area_id: str, types: list[str]) -> list[dict]:
    types = [t for t in types if t in TYPES] or ["restaurant"]
    amenity = [t for t in types if t not in ("bakery",)]
    parts = []
    if amenity:
        parts.append(f'nwr["amenity"~"^({"|".join(amenity)})$"]["name"](area.a);')
    if "bakery" in types:
        parts.append('nwr["shop"~"^(bakery|pastry)$"]["name"](area.a);')
    ql = f'[out:json][timeout:120];{_area_ql(area_id)}({"".join(parts)});out tags center;'
    last_err = None
    import asyncio
    for attempt, url in enumerate(MIRRORS * 2):  # two rounds: public servers are sometimes briefly overloaded
        if attempt >= len(MIRRORS):
            await asyncio.sleep(6)
        try:
            r = await client().post(url, data={"data": ql}, timeout=75)
            if r.status_code == 200 and r.text.lstrip().startswith("{"):
                return [_to_lead(e, area_id) for e in r.json().get("elements", []) if e.get("tags", {}).get("name")]
            last_err = f"HTTP {r.status_code}"
        except Exception as e:
            last_err = e.__class__.__name__
    raise RuntimeError(f"OpenStreetMap nicht erreichbar ({last_err}). Später nochmal probieren.")


def district_from_tags(t: dict, area_id: str) -> str:
    """Whole-area searches: derive the Viennese Bezirk from the postcode (1070 -> 7. Neubau) or use the town."""
    pc = (t.get("addr:postcode") or "").strip()
    if area_id == "wien" and len(pc) == 4 and pc.startswith("1"):
        n = int(pc[1:3])
        if 1 <= n <= 23:
            return f"{n}. {WIEN_BEZIRKE[n - 1]}"
    if area_id == "moedling" and t.get("addr:city") in MOEDLING_GEMEINDEN:
        return t["addr:city"]
    return _label(area_id)


def _to_lead(e: dict, area_id: str) -> dict:
    t = e["tags"]
    from .pipeline import normalize_instagram, normalize_phone
    lat = e.get("lat") or (e.get("center") or {}).get("lat")
    lon = e.get("lon") or (e.get("center") or {}).get("lon")
    street = " ".join(x for x in [t.get("addr:street"), t.get("addr:housenumber")] if x)
    city = " ".join(x for x in [t.get("addr:postcode"), t.get("addr:city")] if x)
    kind_word = TYPES.get(t.get("amenity") or t.get("shop") or "", t.get("amenity") or t.get("shop") or "")
    cuisine = (t.get("cuisine") or "").replace(";", ", ").replace("_", " ")
    return {
        "kind": "restaurant", "name": t["name"], "source": "osm", "source_id": f"{e['type']}/{e['id']}",
        "phone": normalize_phone(t.get("contact:phone") or t.get("phone") or t.get("contact:mobile")),
        "email": (t.get("contact:email") or t.get("email") or "").split(";")[0].strip() or None,
        "website": (t.get("contact:website") or t.get("website") or "").split(";")[0].strip() or None,
        "instagram": normalize_instagram(t.get("contact:instagram") or t.get("instagram")),
        "address": ", ".join(x for x in [street, city] if x) or None, "district": district_from_tags(t, area_id),
        "lat": lat, "lon": lon, "category": ", ".join(x for x in [kind_word, cuisine] if x),
        "extra": {"brand": t.get("brand") or (t.get("brand:wikidata") and t.get("name")), "amenity": t.get("amenity") or t.get("shop"),
                  "cuisine": t.get("cuisine"), "opening_hours": t.get("opening_hours"), "facebook": t.get("contact:facebook") or t.get("facebook"),
                  "outdoor_seating": t.get("outdoor_seating"), "osm": f"https://www.openstreetmap.org/{e['type']}/{e['id']}"},
    }
