#!/bin/bash
# SampleHunter starten: installiert beim ersten Mal die Python-Pakete
# und öffnet die Bibliothek in einem eigenen Fenster.
cd "$(dirname "$0")"
if ! command -v ffmpeg >/dev/null; then
  echo "ffmpeg fehlt. Einmal ausführen:  brew install ffmpeg"
  read -n1 -p "Taste drücken …"; exit 1
fi
if [ ! -d .venv ]; then
  echo "Erster Start: installiere Pakete, das dauert ein paar Minuten …"
  python3 -m venv .venv
  .venv/bin/pip install --upgrade pip
  .venv/bin/pip install -r requirements.txt || exit 1
fi
if [ ! -f .venv/ear-ok ]; then
  echo "Installiere das KI-Ohr (ca. 1 GB, einmalig) …"
  .venv/bin/pip install -r requirements-ear.txt && touch .venv/ear-ok \
    || echo "KI-Ohr ließ sich nicht installieren – SampleHunter läuft trotzdem, nur ohne."
fi
if [ ! -f .venv/app-ok ]; then
  echo "Installiere das App-Fenster (für Drag & Drop in die DAW, einmalig) …"
  .venv/bin/pip install -r requirements-app.txt && touch .venv/app-ok
fi
if [ -f .venv/app-ok ]; then
  .venv/bin/python app.py
else
  echo "App-Fenster nicht verfügbar – starte im Browser (ohne Drag & Drop in die DAW)."
  .venv/bin/python serve.py
fi
