"""Pipeline stages, follow-up cadence, lead scoring, message templates."""
from __future__ import annotations

import re
from datetime import date, timedelta

STAGES = {
    "artist": [
        ("neu", "Neu", "gray"), ("warmup", "Warm-up", "teal"), ("angeschrieben", "Angeschrieben", "blue"),
        ("followup", "Follow-up", "indigo"), ("geantwortet", "Geantwortet", "orange"), ("gespraech", "Call / Treffen", "purple"),
        ("angebot", "Angebot", "purple"), ("gebucht", "Gebucht", "green"), ("geliefert", "Geliefert", "green"),
        ("kein_interesse", "Kein Interesse", "red"), ("spaeter", "Später", "gray"),
    ],
    "restaurant": [
        ("neu", "Neu", "gray"), ("kontaktiert", "Kontaktiert", "blue"), ("followup", "Follow-up", "indigo"),
        ("geantwortet", "Geantwortet", "orange"), ("gespraech", "Gespräch / Testshoot", "purple"),
        ("angebot", "Angebot", "purple"), ("gebucht", "Gebucht", "green"), ("retainer", "Retainer aktiv", "green"),
        ("kein_interesse", "Kein Interesse", "red"), ("spaeter", "Später", "gray"),
    ],
}
STAGES["other"] = STAGES["restaurant"]
CLOSED = {"gebucht", "geliefert", "retainer", "kein_interesse"}

# checkboxes shown in the lead drawer
CHECKS = {
    "artist": [("warmup_done", "Warm-up (gefolgt, 2–3 Likes/Kommentar)"), ("follows_back", "Folgt zurück"),
               ("dm_accepted", "DM-Anfrage angenommen"), ("portfolio_sent", "Portfolio geschickt"),
               ("offer_sent", "Angebot geschickt"), ("deposit_paid", "Anzahlung (50 %) bezahlt"),
               ("delivered", "Geliefert"), ("testimonial", "Testimonial / Repost bekommen"), ("referral_asked", "Nach Empfehlung gefragt")],
    "restaurant": [("visited", "Vor Ort gewesen / gegessen"), ("portfolio_sent", "Beispiele geschickt"),
                   ("test_shot", "Gratis-Testbild gemacht"), ("offer_sent", "Angebot geschickt"),
                   ("deposit_paid", "Anzahlung bezahlt"), ("delivered", "Geliefert"), ("testimonial", "Bewertung / Testimonial"),
                   ("referral_asked", "Nach Empfehlung gefragt")],
}
CHECKS["other"] = CHECKS["restaurant"]

# qualifiers that feed the score (user ticks them after a quick look at the profile)
QUALIFIERS = {
    "artist": [("recent_release", "Release in letzten 60 Tagen / Snippet angekündigt", 15),
               ("posts_often", "Postet ≥ 2× pro Woche", 10), ("weak_videos", "Videos schwach (Handy/Licht) oder keine", 15),
               ("invests", "Merch / Shows / Bookings (investiert Geld)", 10), ("local", "Aus Wien / Mödling / NÖ-Süd", 15)],
    "restaurant": [("new_opening", "Neueröffnung (< 6 Monate)", 20), ("good_rating", "Google ≥ 4,4 bei 100+ Bewertungen", 15),
                   ("few_photos", "Wenige / schlechte Fotos online", 15), ("weak_instagram", "Instagram schwach oder keins", 15),
                   ("delivery", "Auf Lieferando / Wolt / Foodora", 10), ("price_mid", "Preisniveau €€–€€€", 10),
                   ("chain", "Mehrere Standorte (Retainer-Potenzial)", 5)],
}
QUALIFIERS["other"] = QUALIFIERS["restaurant"]

# days until the next follow-up after a contact, by how many contacts happened
CADENCE = {"artist": [4, 7, 60], "restaurant": [4, 6, 20, 60], "other": [4, 6, 20, 60]}

PRICES = {
    "artist": [("Musikvideo Low-Budget", 400, 1000), ("Musikvideo Mid", 1200, 3500),
               ("Visualizer + 3 Reels", 300, 600), ("Reel-Paket (4–6 Clips)", 250, 600)],
    "restaurant": [("Food-Shooting (15–30 Bilder)", 350, 800), ("Food + Interior Paket", 900, 1200),
                   ("Retainer Basic / Monat", 600, 1000), ("Retainer Plus / Monat", 1200, 2000),
                   ("Imagefilm 60–120 s", 1000, 2500)],
}


PHOTOGENIC = ("brunch", "breakfast", "burger", "sushi", "japanese", "ramen", "italian", "pizza", "pasta", "cake", "coffee",
              "café", "cafe", "bakery", "dessert", "ice_cream", "asian", "korean", "thai", "vietnamese", "mexican", "steak",
              "cocktail", "wine", "seafood", "poke", "bowl", "vegan", "fusion", "fine_dining", "regional", "austrian")
CHAINS = ("mcdonald", "burger king", "subway", "starbucks", "kfc", "vapiano", "nordsee", "pizza hut", "domino",
          "dunkin", "aida", "oberlaa", "coffeeshop company", "der mann", "ströck", "anker", "felber", "back-factory",
          "le burger", "five guys", "dean & david", "wienerwald", "spar", "billa", "segafredo", "tchibo")


def stage_ids(kind: str) -> list[str]:
    return [s[0] for s in STAGES.get(kind, STAGES["other"])]


def score(lead: dict) -> int:
    kind = lead.get("kind", "other")
    flags = lead.get("flags") or {}
    pts = 0
    if kind == "artist":
        f = lead.get("followers")
        if f is not None:
            pts += 20 if 2000 <= f <= 30000 else 12 if 30000 < f <= 100000 else 8 if 500 <= f < 2000 else 0
        else:
            pts += 8  # unknown -> neutral
        ml = lead.get("monthly_listeners")
        if ml is not None:
            pts += 15 if 1000 <= ml <= 50000 else 8 if ml > 50000 else 3
        if not lead.get("instagram"):
            pts -= 15
    else:
        extra = lead.get("extra") or {}
        if isinstance(extra, str):
            import json as _j
            try:
                extra = _j.loads(extra)
            except ValueError:
                extra = {}
        pts += 10  # base
        if lead.get("phone") or lead.get("email"):
            pts += 5
        if lead.get("website"):
            pts += 5   # invests in marketing, but often weak visuals
        if not lead.get("instagram"):
            pts += 5   # no IG = need for content
        cuisine = (extra.get("cuisine") or lead.get("category") or "").lower()
        if any(w in cuisine for w in PHOTOGENIC):
            pts += 10
        amenity = extra.get("amenity") or ""
        if amenity in ("restaurant", "cafe", "bar", "ice_cream"):
            pts += 5
        elif amenity == "fast_food":
            pts -= 5
        if extra.get("brand") or any(b in (lead.get("name") or "").lower() for b in CHAINS):
            pts -= 30  # chains decide centrally
    for key, _label, p in QUALIFIERS.get(kind, []):
        if flags.get(key):
            pts += p
    return max(0, min(100, pts))


def next_followup(kind: str, contact_count: int, today: date | None = None) -> str | None:
    seq = CADENCE.get(kind, CADENCE["other"])
    if contact_count <= 0:
        return None
    days = seq[min(contact_count - 1, len(seq) - 1)]
    return ((today or date.today()) + timedelta(days=days)).isoformat()


def normalize_instagram(s: str | None) -> str | None:
    if not s:
        return None
    s = s.strip()
    m = re.search(r"instagram\.com/([A-Za-z0-9_.]{1,30})", s)
    if m:
        s = m.group(1)
    s = s.lstrip("@").strip("/").lower()
    if s in {"p", "reel", "explore", "accounts", "stories", "tv", ""} or not re.fullmatch(r"[a-z0-9_.]{1,30}", s):
        return None
    return s


def normalize_phone(s: str | None) -> str | None:
    if not s:
        return None
    s = s.split(";")[0].strip()
    digits = re.sub(r"[^\d+]", "", s)
    if digits.startswith("00"):
        digits = "+" + digits[2:]
    if digits.startswith("0"):
        digits = "+43" + digits[1:]
    return digits if len(digits) >= 8 else None


DEFAULT_TEMPLATES = [
    # kind, channel, name, subject, body
    ("artist", "instagram", "Erst-DM A – Song-Bezug", "",
     "Yo {vorname}, hab grad „{song}“ gehört – {detail} sitzt brutal. Bin Videograf aus {mein_bezirk} und dreh Musikvideos & Reels für Artists aus Wien. Hab direkt eine Idee, wie man den Track visuell umsetzen könnte ({idee}). Wenn's dich interessiert, schick ich dir ein paar Sachen von mir. Kein Stress 🙏"),
    ("artist", "instagram", "Erst-DM B – Mehrwert", "",
     "Servus {vorname}, feier deinen Sound, vor allem „{song}“. Ich mach Musikvideos & Reels (und nebenbei Beats 🎹). Deine Reels haben mega Potenzial – mit 2–3 Clips aus einem Dreh könntest du den Release viel länger pushen. Darf ich dir zeigen, wie ich das bei anderen gemacht hab?"),
    ("artist", "instagram", "Follow-up (Tag 3–4)", "",
     "Hey {vorname}, wollt nur kurz nachhaken 🙌 Falls du grad an was Neuem sitzt – ich hab im {monat} noch 2 Drehtage frei. Hier ein kurzer Ausschnitt von mir: {portfolio}. Wenn's grad nicht passt, eh easy!"),
    ("artist", "instagram", "Nach Release", "",
     "Glückwunsch zu „{song}“, {vorname}! 🔥 Läuft bei mir grad auf Repeat. Wenn du den Release noch pushen willst: Ich könnt dir an einem halben Tag ein Performance-Video + 3 Reels drehen. Sag Bescheid, dann schick ich dir Ideen."),
    ("restaurant", "instagram", "IG-DM (du)", "",
     "Hallo liebes {name}-Team! Hab euer {gericht} gesehen – schaut richtig gut aus 😍 Ich bin Food- & Content-Creator aus {mein_bezirk} und mach Fotos & Reels für Lokale. Eure Gerichte hätten online viel mehr Reichweite verdient. Hättet ihr Lust auf 2–3 Gratis-Beispielbilder von einem Gericht – ganz unverbindlich?"),
    ("restaurant", "email", "Anfrage per E-Mail (Sie)", "Fotos & Reels für {name} – kurze Idee",
     "Sehr geehrtes {name}-Team,\n\nich bin Videograf und Content Creator aus {mein_bezirk} und war von {name} sehr angetan. Online sieht man davon leider noch wenig – auf Google und Instagram gibt es kaum professionelle Bilder.\n\nIch biete:\n– Food-Fotos für Speisekarte, Google und Lieferplattformen\n– Reels/TikToks für Instagram\n– kurze Imagefilme vom Lokal\n\nBeispiele: {portfolio}\n\nHätten Sie nächste Woche 10 Minuten für ein kurzes Gespräch? Gerne komme ich auch am Nachmittag vorbei.\n\nBeste Grüße\n{mein_name} · {mein_telefon}"),
    ("restaurant", "phone", "Telefon-Skript (30 Sek.)", "",
     "Grüß Gott, {mein_name} hier, Videograf aus {mein_bezirk}. Ganz kurz: Darf ich mit der Person sprechen, die bei Ihnen Social Media macht? … Ich mache Food-Fotos und Reels für Lokale in der Gegend. Mir ist aufgefallen, dass Sie super Bewertungen haben, aber online kaum gute Bilder. Darf ich Ihnen 2–3 Beispiele schicken oder diese Woche am Nachmittag kurz vorbeischauen?\n\n(Beste Zeit: Di–Do 14:30–17:00 – nie während Mittag-/Abendservice)"),
    ("restaurant", "visit", "Walk-in (20 Sek.)", "",
     "Hallo, ich will nicht lang stören – ich bin {mein_name}, Food-Fotograf & Videograf von hier. Mir ist aufgefallen, dass ihr {beobachtung}. Darf ich Ihnen 20 Sekunden zeigen, was ich für andere Lokale mache? [Reel am Handy zeigen] Wer entscheidet bei euch über Fotos & Social Media? Ich lass meine Karte da und meld mich am {tag}."),
    ("restaurant", "email", "Follow-up (Tag 4, anderer Kanal)", "Kurzes Beispiel für {name}",
     "Hallo {name}-Team,\n\nich hatte mich letzte Woche wegen Fotos & Reels gemeldet. Als kleine Idee hier ein Beispiel, wie Ihr {gericht} auf Instagram oder Wolt wirken könnte: {portfolio}\n\nWenn das Thema gerade passt, freue ich mich über einen kurzen Termin – sonst melde ich mich gern im {monat} wieder.\n\nBeste Grüße\n{mein_name}"),
]

MONTHS = ["Jänner", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"]


def render(body: str, lead: dict, me: dict, extra: dict | None = None) -> str:
    first = (lead.get("name") or "").split()[0] if lead.get("kind") == "artist" else lead.get("name", "")
    nxt = MONTHS[date.today().month % 12]
    values = {
        "name": lead.get("name", ""), "vorname": first, "bezirk": lead.get("district") or "",
        "mein_name": me.get("name") or "[DEIN NAME]", "mein_bezirk": me.get("bezirk") or "Mödling",
        "mein_telefon": me.get("phone") or "[TELEFON]", "portfolio": me.get("portfolio") or "[PORTFOLIO-LINK]",
        "monat": nxt,
        **(extra or {}),
    }
    return re.sub(r"\{(\w+)\}", lambda m: values.get(m.group(1), m.group(0)), body)
