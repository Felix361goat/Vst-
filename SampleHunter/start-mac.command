#!/bin/bash
# SampleHunter starten. Beim ersten Mal werden ca. 300 MB installiert
# (App-Fenster und Basis); danach startet es in Sekunden.
cd "$(dirname "$0")"
if ! command -v python3 >/dev/null; then
  echo "Python fehlt. Einmal ausführen:  brew install python"
  read -n1 -p "Taste drücken …"; exit 1
fi
if [ ! -d .venv ]; then
  echo "Erster Start: installiere SampleHunter (ca. 300 MB, ein paar Minuten) …"
  python3 -m venv .venv
  .venv/bin/pip install --upgrade pip -q
fi
if [ ! -f .venv/base-ok-2 ]; then
  .venv/bin/pip install -r requirements.txt && touch .venv/base-ok-2 || exit 1
fi
if [ ! -f .venv/app-ok ]; then
  .venv/bin/pip install -r requirements-app.txt && touch .venv/app-ok
fi
if [ -f .venv/app-ok ]; then
  .venv/bin/python app.py
else
  echo "App-Fenster nicht verfügbar – starte im Browser (ohne Drag & Drop in die DAW)."
  .venv/bin/python serve.py
fi
