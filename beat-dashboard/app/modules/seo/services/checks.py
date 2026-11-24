"""Pre-upload checks: policy risks + best practices. Each check: level ok|warn|block."""
from __future__ import annotations

import re

from .knowledge import BANNED_TITLE_WORDS, HEAD_TERMS, bpm_fit


def run(*, title: str, description: str, tags: list[str], artists: list[str], genre: str, bpm: float | None,
        free: str, sample_source: str, subs: int, store_link: str) -> list[dict]:
    res = []

    def add(level, text):
        res.append({"level": level, "text": text})

    tl = title.lower()
    bad = [w for w in BANNED_TITLE_WORDS if re.search(rf"(^|\W){re.escape(w)}(\W|$)", tl)]
    if bad:
        add("block", f"Titel enthält irreführende Wörter ({', '.join(bad)}) - Verstoß gegen YouTube-Spam-Richtlinien.")
    else:
        add("ok", "Keine irreführenden Wörter (ft., leak, official …) im Titel.")
    if len(title) > 100:
        add("block", f"Titel zu lang ({len(title)}/100 Zeichen).")
    elif len(title) > 80:
        add("warn", f"Titel ist {len(title)} Zeichen - ideal sind 55-80.")
    else:
        add("ok", f"Titellänge {len(title)} Zeichen.")
    pos = tl.find("type beat")
    if pos >= 0 and pos + 9 > 45:
        add("warn", "'… Type Beat' endet erst nach Zeichen 45 - auf dem Handy evtl. abgeschnitten. Keyword nach vorne!")
    if tl.count("type beat") > 1:
        add("warn", "'Type Beat' kommt mehrfach im Titel vor - einmal reicht.")
    if len(artists) >= 3:
        add("block", "3+ Künstler im Titel verwässern das Keyword. Max. 2 mit 'x'.")
    if subs < 100_000:
        heads = [a for a in artists[:1] if a.lower() in HEAD_TERMS]
        if heads:
            add("warn", f"'{heads[0]} Type Beat' ist extrem umkämpft. Unter ~100k Abos lieber Kombo mit kleinerem Künstler oder Subgenre-Titel.")
    if "[free]" in tl or "(free)" in tl:
        if free == "none":
            add("block", "Titel sagt FREE, aber die Lizenz ist nicht frei - irreführend.")
        else:
            add("ok", "FREE-Tag passt zur Lizenzangabe.")
    first150 = description[:150]
    if store_link and store_link not in first150:
        add("warn", "Store-Link sollte in den ersten 150 Zeichen der Beschreibung stehen.")
    if not store_link:
        add("warn", "Kein Store-Link eingetragen (Einstellungen) - ohne Link keine Verkäufe.")
    hashtags = re.findall(r"#\w+", description)
    if len(hashtags) > 15:
        add("block", f"{len(hashtags)} Hashtags - über 15 wirkt spammy, über 60 ignoriert YouTube alle.")
    elif len(hashtags) > 5:
        add("warn", f"{len(hashtags)} Hashtags - 3 sind optimal (die ersten 3 erscheinen über dem Titel).")
    if len(description) > 5000:
        add("block", f"Beschreibung zu lang ({len(description)}/5000).")
    chars = sum(len(t) + (2 if " " in t else 0) + 1 for t in tags)
    if chars > 500:
        add("block", f"Tags zu lang ({chars}/500 Zeichen).")
    else:
        add("ok", f"Tags: {len(tags)} Stück, {chars}/500 Zeichen.")
    if sample_source == "uncleared":
        add("block", "Ungeklärte Samples: Content-ID-Claim-Gefahr und du darfst den Beat nicht kommerziell lizenzieren.")
    elif sample_source == "royalty_free":
        add("warn", "Royalty-free Samples: Lizenz des Packs prüfen (manche Splice-/Loop-Lizenzen schließen Content ID / Exklusiv-Verkauf aus).")
    fit = bpm_fit(genre, bpm)
    if fit:
        add("ok" if fit["ok"] else "warn", fit["message"])
    return res


UPLOAD_CHECKLIST = [
    "Thumbnail 1280×720, dunkel/kontrastreich, 1–3 Wörter (z.B. Künstlername oder 'DARK RAGE'), Logo immer in derselben Ecke",
    "Keine Künstlerfotos/Albumcover ohne Rechte - lieber Illustration, Silhouette oder lizenziertes Artwork",
    "Beat startet in den ersten 0–20 Sek. (kein langes Intro), Producer-Tag max. 1× in den ersten 30 Sek.",
    "Länge 2:30–3:30, alle 4–8 Takte eine Veränderung",
    "Kategorie 'Musik', Sprache & Untertitel-Sprache auf Englisch",
    "Zu 2 Playlists hinzufügen (Künstler-Playlist + Subgenre-Playlist)",
    "Endbildschirm (letzte 20 Sek.): bester Beat + Künstler-Playlist + Abonnieren",
    "Karte bei ~50 % auf einen ähnlichen Beat",
    "Kommentar mit Store-Link anpinnen",
    "1–2 Shorts (15–30 Sek. bester 8-Takt-Loop, 9:16) mit Link zum vollen Beat (Related Video)",
    "Gleicher Titel, Tags und BPM/Key auf BeatStars/Airbit",
    "Bei 3 Thumbnails: 'Testen & vergleichen' in YouTube Studio nutzen",
    "Geleaste Beats NICHT in Content ID registrieren (sonst werden Songs deiner Kunden geclaimt)",
    "Check nach 48 h: CTR (Ziel 4–8 %) · nach 7 Tagen: Wiedergabedauer (Ziel 40 %+) · nach 30 Tagen: Verkäufe",
]
