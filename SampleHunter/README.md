# SampleHunter

Gräbt automatisch unbekannte Aufnahmen aus, findet darin das eine Riff, das
hängen bleibt, schneidet es taktgenau heraus und legt es in eine Bibliothek,
die sich anfühlt wie Splice: anhören, filtern, liken, herunterladen.

Vorbild ist das Tumbi-Riff aus *Get Ur Freak On*: kurz, wiederholt sich,
ein einzelnes gezupftes Instrument, synkopiert, nasal-hell, von irgendwo, wo
sonst keiner sucht. Das Instrument und die Herkunft sind egal. Es zählt nur,
ob die Stelle so funktioniert.

## Starten

**Windows:** einmal `winget install Gyan.FFmpeg` und [Python 3](https://python.org)
installieren, dann `start-windows.bat` doppelklicken.

**macOS:** einmal `brew install ffmpeg python`, dann `start-mac.command` doppelklicken.

Beim ersten Start werden die Python-Pakete installiert (dauert ein paar Minuten),
danach öffnet sich die Bibliothek unter <http://localhost:8765>. Unten auf **Los**
klicken, dann beginnt die Suche. Pro Aufnahme dauert das etwa 20–40 Sekunden.
Neue Samples erscheinen live in der Liste.

Ohne Browser geht es auch:

```bash
python hunt.py --count 50                       # 50 neue Samples
python hunt.py --terms "kora,banjo,gamelan"     # nur bestimmte Instrumente/Stile
python hunt.py --allow-nc                       # auch CC BY-NC (nur nicht-kommerziell)
python hunt.py --include-claimed                # auch Archive-Uploads mit ungeprüfter Lizenz
python serve.py                                 # Bibliothek öffnen
```

## Die Bibliothek

| | |
|---|---|
| **Leertaste** | abspielen / stoppen |
| **↑ ↓** | nächstes Sample (spielt direkt weiter, wenn schon etwas läuft) |
| **L / D** | Like / Nicht mein Ding |
| **/** | Suche |
| Klick in die Wellenform | ab dieser Stelle abspielen |
| **Sync** + BPM | spielt im Projekttempo, wie eine schneller gedrehte Platte; Tonhöhe geht mit |
| **Pitch** | in Halbtönen transponieren |
| Name ziehen | in Explorer/Finder ziehen (Chrome/Edge) |
| **Likes als ZIP** | alle gelikten WAVs plus `CREDITS.txt` mit Quellen und Lizenzen |

Jedes Sample ist eine 24-Bit-WAV-Datei bei 44,1 kHz, normalisiert auf −1 dBFS,
auf dem Beat geschnitten und mit 4-ms-Fades. Sie loopt also bei der BPM, die
im Namen steht. Der Link-Button führt zur Originalaufnahme mit Zeitangabe, falls
du mehr davon willst.

## Wie es sucht

**Quellen.** Drei, sortiert danach, wie sehr man der Lizenz trauen kann:

1. **78er-Schellackplatten** von vor 1926 (Internet Archive, Great 78 Project,
   Library of Congress). In den USA sind Aufnahmen gemeinfrei, die vor mehr als
   100 Jahren erschienen sind. Für Kompositionen gilt das nach 95 Jahren. Die
   Grenze verschiebt sich jeden Januar automatisch um ein Jahr. Hawaii-Gitarre,
   Banjo, Xylophon, Balalaika, arabische, türkische und japanische Aufnahmen …
   Ein Teil der Suche zieht völlig zufällig aus den rund 67.000 Platten dieser Zeit.
2. **Wikimedia Commons**: ethnografische Aufnahmen und Instrumenten-Demos
   (Gamelan, Kora, Mbira …), Lizenzen von der Community geprüft.
3. **Andere Archive-Uploads** mit freier Lizenz: nur mit `--include-claimed`,
   weil manche Uploader „Public Domain“ auf kommerzielle Platten schreiben.

Suchbegriffe sind über 130 Instrumente und Stile aus aller Welt (siehe
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
serve.py           die Bibliothek im Browser
hunter/sources.py  Quellen, Suchbegriffe, Lizenzregeln
hunter/analyze.py  Riff-Erkennung und Freak-Score
hunter/library.py  Bibliothek und Geschmacks-Lernen
web/index.html     die Oberfläche
library/           deine Samples (wird angelegt, nicht im Git)
```
