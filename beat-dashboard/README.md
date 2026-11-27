# 🎛️ Beat Dashboard

Dein lokales Dashboard für Beat-Kanal und Video-Business. Es läuft auf deinem PC im Browser, und deine Daten und Keys bleiben bei dir.

**Modul 1: YouTube SEO für Type Beats.** So funktioniert es:

1. Du ziehst deinen Beat rein, oder du sagst grob, was es ist (Genre, Mood, Künstler).
2. Das Dashboard erkennt **BPM und Tonart** lokal.
3. Es recherchiert über externe Quellen, welche Keywords sich lohnen (Nachfrage gegen Konkurrenz).
4. Es schreibt **Titel, Tags, Hashtags, Beschreibung, angepinnten Kommentar und Short-Titel**.
5. Es prüft alles gegen die YouTube-Richtlinien.

---

## 🚀 Starten (Windows)

1. **Python installieren** (einmalig): https://www.python.org/downloads/. Beim Installer unbedingt **„Add python.exe to PATH“** anhaken.
2. Den Ordner `beat-dashboard` irgendwo hinkopieren (oder das ganze Repo klonen).
3. **Doppelklick auf `start.bat`.**
   - Beim ersten Mal installiert es alles selbst (1–2 Minuten).
   - Danach öffnet sich der Browser auf http://127.0.0.1:8765.
   - Das schwarze Fenster offen lassen, solange du arbeitest.

macOS/Linux: `./start.sh`

## 🔑 Keys (alle optional)

Ohne Keys funktioniert schon viel. Unter **Einstellungen** im Dashboard eintragen und mit „Testen“ prüfen:

| Quelle | Kosten | Was sie bringt |
|---|---|---|
| YouTube-Autocomplete | gratis, kein Key | Echte Suchanfragen und Nachfrage-Signal, Long-Tail-Keywords |
| Deezer | gratis, kein Key | Ähnliche Künstler (für „x“-Kombos), Fan-Zahlen |
| kworb.net | gratis, kein Key | Künstler, deren Hörerzahlen gerade steigen |
| **YouTube Data API v3** | **gratis** (Key nötig) | **Echte Konkurrenz**: Uploads der letzten 30 Tage, Views, Kanalgrößen, Outlier, Tags der Konkurrenz, Upload-Zeiten. Sehr empfohlen. |
| Last.fm | gratis (Key nötig) | Bessere ähnliche Künstler und Hörerzahlen |
| Claude (Anthropic) | kostenpflichtig, Cent-Beträge | Zusätzliche KI-Titel, Beschreibung, Thumbnail-Text, Strategie-Tipp |

**YouTube-Key holen (5 Minuten):**

1. https://console.cloud.google.com öffnen und ein neues Projekt anlegen.
2. Unter „APIs & Dienste“ → „Bibliothek“ nach **YouTube Data API v3** suchen und **Aktivieren**.
3. „Anmeldedaten“ → „Anmeldedaten erstellen“ → **API-Schlüssel**.
4. Den Schlüssel im Dashboard unter Einstellungen einfügen.

> Seit 2026 erlaubt Google ca. **100 Suchen pro Tag**. Eine Analyse mit Tiefe 4 braucht 8 Suchen. Ergebnisse werden 2 Tage zwischengespeichert, Wiederholungen kosten also nichts. Den Verbrauch siehst du unten links.

Außerdem in den Einstellungen eintragen:
- **Produzentenname**
- **Store-Link** (BeatStars/Airbit)
- **Instagram/Kontakt**
- **Abozahl**: damit die Empfehlungen zu deiner Kanalgröße passen. Kleine Kanäle sollten „Drake Type Beat“ meiden.

## 🎯 Was das SEO-Modul kann

- **Neuer Beat**:
  - Audio-Analyse: BPM mit Halb- und Doppel-Alternative, Tonart, Camelot.
  - Keyword-Chancen-Tabelle mit Score von 0 bis 100. Mit „Verwenden“ schaltest du auf ein besseres Keyword um.
  - Titel mit Live-Prüfung, Tags (≤ 500 Zeichen, richtig sortiert), Beschreibung.
  - Upload-Zeit-Analyse, Outlier-Videos zum Abschauen.
- **Keyword-Explorer**: Autocomplete, Long-Tail (a–z), YouTube-Nischen-Check für einzelne Phrasen.
- **Verlauf**: Alle Analysen werden gespeichert (`data/dashboard.db`).
- **Uploads & Performance**: YouTube-Links eintragen, Views abrufen (1 Einheit pro 50 Videos). Zeigt, welche Keywords für **dich** funktionieren.
- **Checkliste & Wissen**: Upload-Checkliste zum Abhaken, Genre-, BPM- und Künstler-Tabelle, Titel-Regeln.

**So entsteht der Chance-Score:**
- **Nachfrage**: Taucht „… type beat“ schon beim Tippen des Namens auf? Dazu Popularität und Views.
- **Konkurrenz**: Uploads der letzten 30 Tage (unter 50 offen, ab 1000 gesättigt), Kanalgrößen der Top 10, exaktes Keyword im Titel.
- **Bonus** für frische Top-Videos, Outlier und steigende Künstler.

Die Regeln und Genre-Daten stehen in `app/modules/seo/services/knowledge.py` und dürfen gern angepasst werden.


## 🤝 Leads & CRM

Hier findest du **Rapper/Artists** und **Restaurants** in Wien und im Bezirk Mödling. Du schreibst sie an und trackst alles bis zur Buchung.

**Lead-Finder**
- **Restaurants** kommen aus OpenStreetMap (gratis): Name, Adresse, Telefon, Website, Instagram, Küche.
  - Du wählst ganz Wien, einzelne Wiener Bezirke oder Gemeinden im Bezirk Mödling.
  - Optional durchsucht das Dashboard die Website und die **Impressum-/Kontaktseite** nach E-Mail und Instagram.
- **Artists** kommen aus MusicBrainz und Wikidata (gratis). Mit YouTube-Key kommen **Newcomer** über ihre YouTube-Kanäle dazu: Die Kanalbeschreibung enthält meist den Instagram-Namen und eine Booking-Mail.
- **Copy & Paste:** Instagram-Profile, die du selbst findest (#wienrap, #austrorap …), einfach als Liste einfügen.
- CSV-Import und -Export funktionieren mit Excel.

**CRM**
- **Heute:** fällige Follow-ups, beste neue Leads, Antwortquote, Umsatz, DM-Zähler (max. ~15 pro Tag).
- **Pipeline:** Kanban-Board. Karten per Drag & Drop in die nächste Spalte ziehen.
- **Liste:** Filter, Suche, Mehrfachauswahl. Ein Klick auf **„✓ Kontaktiert“** oder **„↩︎ Antwort“** reicht.
  - Das Dashboard setzt dann den Status und plant das nächste Follow-up selbst.
  - Takt bei Artists: Tag 4, Tag 7, dann 60 Tage Pause.
- **Lead-Details:**
  - Checkliste (Warm-up, Portfolio geschickt, Anzahlung …) und Qualifizierung, die den Score erhöht.
  - Deal-Wert, Notizen, Verlauf.
  - Nachricht aus einer Vorlage, mit Platzhaltern wie `{song}`, die du persönlich ausfüllst.
  - **„Kopieren & als kontaktiert markieren“** öffnet gleich das Instagram-Profil.
- **Sperrliste:** Wer keinen Kontakt will, wird nie wieder vorgeschlagen, auch nicht bei neuen Suchen.

**⚖️ Wichtig (Österreich, § 174 TKG):**
- Werbe-E-Mails und Werbeanrufe brauchen **vorher eine Einwilligung, auch bei Firmen**.
- Bei Restaurants daher: **vorbeigehen** (Di–Do 14:30–17:00) oder einen Brief schicken, dort fragen, ob du Beispiele mailen darfst, und *dann* mailen.
- Instagram-DMs nur persönlich und 1:1, keine Massennachrichten.
- Details stehen im Tab „Regeln & Tipps“. Das ist keine Rechtsberatung.

In den Einstellungen trägst du deinen Namen, deinen Ort, deine Telefonnummer und deinen Portfolio-Link für die Vorlagen ein.

## 🧩 Später erweitern (eigene Module)

Jedes Modul ist ein Ordner in `app/modules/`. Das Dashboard findet ihn beim Start automatisch und zeigt ihn im Menü an. Am Kern muss nichts geändert werden.

```
app/modules/katalog/
  __init__.py     # MODULE = {...}
  router.py       # API-Endpunkte  -> /api/katalog/...
  schema.sql      # eigene Tabellen (CREATE TABLE IF NOT EXISTS ...)
  page.js         # Oberfläche     -> export default { mount(el, ctx) {...} }
```

`__init__.py`:
```python
from .router import router

MODULE = {
    "id": "katalog", "name": "Beat-Katalog", "icon": "💿", "order": 20,
    "description": "Alle Beats, Lizenzen, Verkäufe",
    "router": router, "page": "page.js", "schema": "schema.sql",
    "settings": [{"key": "BEATSTARS_USER", "label": "BeatStars-Name", "secret": False}],
}
```

`router.py`:
```python
from fastapi import APIRouter
from ... import db
router = APIRouter()

@router.get("/beats")
def beats():
    with db.tx() as c:
        return [dict(r) for r in c.execute("SELECT * FROM beats")]
```

`page.js`:
```js
import { api, html } from '/static/api.js';
export default {
  async mount(el) {
    const beats = await api('/api/katalog/beats');
    el.innerHTML = html`<h1>💿 Beat-Katalog</h1><p>${beats.length} Beats</p>`;
  },
};
```

Ideen für weitere Module, wenn das Business wächst:
- Beat-Katalog (die Tabellen `analyses` und `uploads` haben schon eine `beat_id`-Spalte zum Verknüpfen)
- Lizenz- und Verkaufs-Tracking
- Einnahmen/Ausgaben
- Social-Media-Planer (Shorts, TikTok, Instagram)
- Kunden/Artists-CRM
- Release-Kalender der Künstler

Module lassen sich unter Einstellungen → Module ein- und ausschalten.

## 📁 Aufbau

```
beat-dashboard/
  start.bat / start.sh     Ein-Klick-Start
  .env                     deine Einstellungen und Keys (wird nicht hochgeladen)
  data/                    Datenbank, Cache, hochgeladene Beats (wird nicht hochgeladen)
  app/
    main.py, registry.py   Kern + automatische Modul-Erkennung
    core_api.py            Einstellungen, Status, Kontingent
    cache.py               Cache und YouTube-Kontingent-Zähler
    modules/seo/           das YouTube-SEO-Modul
  static/                  Oberfläche (HTML/CSS/JS, kein Build nötig)
```

API-Dokumentation zum Ausprobieren: http://127.0.0.1:8765/docs

## ⚠️ Hinweise

- Genauere BPM- und Tonarterkennung: `.venv\Scripts\pip install -r requirements-extra.txt` (installiert librosa). Das Dashboard nutzt es dann automatisch.
- MP3, WAV, FLAC, OGG und AIFF werden unterstützt. Bei M4A/AAC vorher nach WAV exportieren.
- Die YouTube-Autocomplete-Schnittstelle ist inoffiziell. Das Dashboard fragt sie gedrosselt ab und speichert die Antworten zwischen.
- Künstlernamen nur beschreibend verwenden („Type Beat“). Keine Künstlerfotos oder Albumcover ohne Rechte, keine „ft.“- oder „leak“-Titel. Das Dashboard warnt dich davor.
