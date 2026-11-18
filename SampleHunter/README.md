# SampleHunter

Gräbt automatisch unbekannte Aufnahmen aus, findet darin das eine Riff, das
hängen bleibt, schneidet es taktgenau heraus und legt es in eine Bibliothek,
die sich anfühlt wie Splice: anhören, filtern, liken, herunterladen.

Vorbild ist das Tumbi-Riff aus *Get Ur Freak On*: kurz, wiederholt sich,
ein einzelnes gezupftes Instrument, synkopiert, nasal-hell, von irgendwo, wo
sonst keiner sucht. Das Instrument und die Herkunft sind egal. Es zählt nur,
ob die Stelle so funktioniert.

Es gibt zwei Arten von Samples:

- **Loops** (rot markiert): Riffs, auf dem Beat geschnitten, loopen bei der BPM im Namen.
- **Phrasen** (blau markiert): freie Linien, gesungen oder gespielt, mit Verzierungen
  und Glissandi, als One-Shot. So etwas wie die karnatische Sängerin, die seit
  Jahrzehnten in Hip-Hop-Beats auftaucht.

Es ist ein eigenes Programm mit eigenem Fenster, Claude braucht es nicht.

**Die Bibliothek liegt in der Cloud, nicht auf deiner Festplatte.** Jedes
Sample ist nur ein Verweis: welche Aufnahme (bei archive.org oder Wikimedia),
ab welcher Sekunde, wie lang. Erst beim Anhören werden genau diese Sekunden
geladen (ca. 2 s, danach aus dem Cache). Eine Datei auf der Festplatte gibt es
nur für:

- **angehörte Samples**: ein Cache, der sich bei 400 MB selbst aufräumt;
- **Favoriten (♥)**: in voller Qualität gesichert, falls die Quelle mal verschwindet;
- **was du in FL Studio ziehst oder herunterlädst**: liegt in `Musik/SampleHunter`
  und bleibt dort, weil dein FL-Projekt auf genau diese Datei zeigt.

Oben rechts steht, wie viel SampleHunter lokal belegt. Mit der App kommt ein
fertiger **Katalog** (`catalog/`), du musst also nicht erst suchen. Deine
eigenen Suchen kommen dazu.
Samples ziehst du direkt aus dem Fenster in FL Studio (oder jede andere DAW),
gespeichert wird als MP3.

## Starten

**Windows:** einmal in der Eingabeaufforderung

```
winget install Gyan.FFmpeg
winget install Python.Python.3.12
```

dann `start-windows.bat` doppelklicken.

**macOS:** einmal `brew install ffmpeg python`, dann `start-mac.command` doppelklicken.

Beim ersten Start werden die Python-Pakete installiert (dauert ein paar Minuten,
inklusive KI-Ohr ca. 1 GB; das Modell wird bei der ersten Suche einmal geladen),
danach öffnet sich SampleHunter als eigene App. Fenster zu = Programm aus. Unten auf **Los**
klicken, dann beginnt die Suche. Pro Aufnahme dauert das etwa 20–40 Sekunden.
Neue Samples erscheinen live in der Liste.

Ohne Browser geht es auch:

```bash
python hunt.py --count 50                       # 50 neue Samples
python hunt.py --terms "kora,banjo,gamelan"     # nur bestimmte Instrumente/Stile
python hunt.py --allow-nc                       # auch CC BY-NC (nur nicht-kommerziell)
python hunt.py --include-claimed                # auch Archive-Uploads mit ungeprüfter Lizenz
python hunt.py --experiment                     # auch obskure Uploads mit ungeklärter Lizenz
python hunt.py --kinds phrase                   # nur Phrasen
python hunt.py --store file                     # jedes Sample als Datei behalten statt Cloud
python app.py                                   # die App (mit Drag & Drop in die DAW)
python serve.py                                 # ohne App-Paket: im Browser-Fenster
```

## Die Bibliothek

| | |
|---|---|
| **Leertaste** | abspielen / stoppen |
| **↑ ↓** | nächstes Sample (spielt direkt weiter, wenn schon etwas läuft) |
| **♥ / L** | als Favorit speichern; „♥ Favoriten“ oben zeigt nur die |
| **✕ / X** | ablehnen: weg für immer, taucht nie wieder auf (auch nicht in späteren Suchen) und zählt als „nicht mein Geschmack“ |
| **S** / Wellen-Button | ähnliche Samples finden (Esc = zurück) |
| Klick auf den Titel | alles aus derselben Aufnahme |
| **/** | Suche |
| Klick in die Wellenform | ab dieser Stelle abspielen |
| **Sync** + BPM | spielt im Projekttempo, wie eine schneller gedrehte Platte; Tonhöhe geht mit |
| **Pitch** | in Halbtönen transponieren |
| **Wellenform oder Name ziehen** | Sample direkt in FL Studio (Playlist, Channel Rack, Sampler …) oder in den Explorer ziehen |
| **Format** MP3/WAV | in welchem Format gezogen und heruntergeladen wird (MP3 klein, WAV loopt garantiert lückenlos) |
| Ordner-Button | Sample in eine Sammlung legen („Beat für X“, „Vocals“ …) |
| Sammlungs-Menü oben | nur diese Sammlung zeigen; dann lädt der ZIP-Button sie komplett herunter |
| **Favoriten als ZIP** | alle Favoriten plus `CREDITS.txt` mit Quellen und Lizenzen |
| **Autopilot** (unten) | gräbt immer weiter, solange SampleHunter offen ist |

Gespeichert wird als MP3 mit 320 kbps (`--format wav` für 24-Bit-WAV), normalisiert auf −1 dBFS,
auf dem Beat geschnitten und mit 4-ms-Fades. Sie loopt also bei der BPM, die
im Namen steht. Der Link-Button führt zur Originalaufnahme mit Zeitangabe, falls
du mehr davon willst.

## Wie es sucht

**Quellen.** Drei, sortiert danach, wie sehr man der Lizenz trauen kann:

1. **78er-Schellackplatten** von vor 1926 (nur mit `--vintage`, weil sie knistern) (Internet Archive, Great 78 Project,
   Library of Congress). In den USA sind Aufnahmen gemeinfrei, die vor mehr als
   100 Jahren erschienen sind. Für Kompositionen gilt das nach 95 Jahren. Die
   Grenze verschiebt sich jeden Januar automatisch um ein Jahr. Hawaii-Gitarre,
   Banjo, Xylophon, Balalaika, arabische, türkische und japanische Aufnahmen …
   Ein Teil der Suche zieht völlig zufällig aus den rund 67.000 Platten dieser Zeit.
2. **Wikimedia Commons**: ethnografische Aufnahmen und Instrumenten-Demos
   (Gamelan, Kora, Mbira …), Lizenzen von der Community geprüft.
3. **Andere Archive-Uploads** mit freier Lizenz: nur mit `--include-claimed`,
   weil manche Uploader „Public Domain“ auf kommerzielle Platten schreiben.
4. **Experiment-Modus** (`--experiment`, in der Oberfläche standardmäßig an):
   obskure Archive-Uploads, egal welche Lizenz: Konzertmitschnitte, Kassetten,
   alte Filmsong-Platten, Feldaufnahmen. Nur Aufnahmen mit wenigen Downloads,
   und weltbekannte Namen (Lata Mangeshkar, Nusrat, Bollywood-Labels …) werden
   übersprungen. Diese Samples tragen ein rotes Badge „Lizenz unklar“: zum
   Rumprobieren gut, vor einem Release klären.

Damit keine Sammlung die Bibliothek füllt, kommen aus einer Serie oder von
einem Künstler höchstens acht Samples (`--per-family`). Lange Aufnahmen
(Konzerte bis eine Stunde) werden an einer zufälligen Stelle analysiert.

Suchbegriffe sind rund 270 Instrumente, Stile und Gesangstraditionen aus aller
Welt, die schrägen (Ektara, Gopichand, Morchang, Khaen, Mundbogen, Zar-Ritual,
Obertongesang …) werden doppelt so oft gezogen (siehe
`hunter/sources.py`). Bevorzugt werden Aufnahmen, die kaum jemand
heruntergeladen hat.

**Der Freak-Score (0–100)** bewertet jede mögliche Stelle von ein bis vier Takten:

| Merkmal | was es misst |
|---|---|
| Wiederholung | Die Figur kommt im nächsten Durchgang wieder (Tonhöhen und Rhythmus). Die Loop-Länge wird aus der Periode der Musik bestimmt, nicht geraten. |
| Bewegung | 3–6 verschiedene Töne: eine Melodie, kein Bordun, kein Solo |
| Klarheit | eine hörbare Stimme statt eines Akkordbreis |
| Synkopen | Anschläge zwischen den Zählzeiten |
| Punch | klare Attacks |
| Helligkeit | nasaler Mittenbereich um 1,8 kHz, wo Tumbi, Banjo und Kora sitzen |
| Harmonisch/perkussiv | gezupft: Anschlag plus Ton |
| Tonalität | Rauschen, Knistern und Sprache fallen raus |

**Das KI-Ohr.** Ein Audio-Sprachmodell (CLAP) hört sich jeden Kandidaten an:

- **Grain**: klingt es nach Knistern, Rauschen, dumpfer alter Platte oder
  ausgeleierter Kassette? Über 55 fliegt es raus (`--max-grain`).
- **Weirdness**: klingt es eher nach „hypnotisches Riff auf einem
  ungewöhnlichen Instrument, nasal, schnarrend, metallisch, ritueller Trance“
  oder eher nach „normaler Folk mit Gitarre, Blaskapelle, Tanzorchester,
  Kirchenchor, Pop“? Zählt zu 60 % in den Score; unter 45 fliegt es raus (`--min-weird`).
- **Klang-Fingerabdruck**: damit funktionieren „Ähnliche finden“, das
  Geschmacks-Lernen und die **Suche nach Klang**: beschreib oben einen Sound
  („eerie nasal riff“, „buzzing reed drone“, „metallic bells“, Englisch klappt
  am besten) und drück 🔊.

Ohne KI-Ohr (z. B. wenn torch nicht installiert werden kann) läuft alles
weiter, nur mit der Signal-Analyse.

**Phrasen** werden anders bewertet: eine klare Stimme, die frei steht,
Bewegung (Verzierungen, Glissandi), ein Ambitus von ein paar Tönen, gehaltene
Töne dazwischen, sauberer Anfang und Ende, 1,5–8 Sekunden.

**Sauberer Klang.** Drei Prüfungen, bevor ein Sample in die Bibliothek darf:
das KI-Ohr (Grain unter 45), ein Klick-Detektor im Signal selbst (unter 30
Knackser pro Sekunde; alte Platten liegen bei 50–200), und für den Katalog
noch einmal strenger (Grain unter 40).

**Dein Geschmack.** Jeder Like und Dislike fließt in die nächste Suche ein:
Suchbegriffe, deren Samples du magst, werden öfter gezogen. Ab drei
Bewertungen bekommt jedes neue Sample zusätzlich bis zu ±10 Punkte dafür, wie
nah es an deinen Likes klingt. In der Liste steht das als grüne/gelbe Zahl
neben dem Score, und „Freak + Geschmack“ sortiert danach.

## Lizenzen, ehrlich

Die Badges in der Liste sagen, was du darfst:

- **Public Domain / PD USA**: frei, auch kommerziell. „PD USA“ heißt: in den
  USA sicher. In anderen Ländern können für alte Aufnahmen andere Fristen
  gelten (in der EU z. B. 70 Jahre ab Veröffentlichung, also auch erledigt, aber
  Remasterings können eigene Rechte haben).
- **CC BY**: frei, du musst die Quelle nennen. Das steht in `CREDITS.txt`.
- **CC BY-SA**: Quelle nennen, *und* dein Track müsste selbst unter CC BY-SA
  stehen. Für kommerzielle Releases meistens ungeeignet; zum Rumprobieren gut.
- **CC BY-NC**: nur nicht-kommerziell. Standardmäßig aus.

Alles mit „ND“ (keine Bearbeitungen) oder ohne Lizenz wird nie verwendet.
„geprüft“ heißt, die Lizenz folgt aus dem Alter oder wurde von Wikimedia
geprüft. „Angabe des Uploaders“ heißt: vor einem Release selbst nachsehen.
Das hier ist keine Rechtsberatung. Bei einem Release mit Label lohnt ein Blick
auf die Quellseite.

## Dateien

```
hunt.py            die Suche (Kommandozeile)
app.py             die App: eigenes Fenster, Drag & Drop in die DAW
serve.py           die Bibliothek (Server; auch allein im Browser nutzbar)
hunter/sources.py  Quellen, Suchbegriffe, Lizenzregeln
hunter/analyze.py  Riff-Erkennung und Freak-Score
hunter/library.py  Bibliothek, Katalog, Favoriten, Ablehnen, Geschmacks-Lernen
hunter/cloud.py    Samples aus der Cloud holen, Cache und gesicherte Favoriten
hunter/ear.py      das KI-Ohr
catalog/           der mitgelieferte Katalog (nur Verweise, ein paar KB pro Sample)
tools/build_catalog.py  macht aus Suchergebnissen einen Katalog
web/index.html     die Oberfläche
library/           deine Samples (wird angelegt, nicht im Git)
```
