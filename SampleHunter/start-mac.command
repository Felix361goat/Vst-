#!/bin/bash
# SampleHunter starten: installiert beim ersten Mal die Python-Pakete
# und öffnet die Bibliothek im Browser.
cd "$(dirname "$0")"
if ! command -v ffmpeg >/dev/null; then
  echo "ffmpeg fehlt. Einmal ausführen:  brew install ffmpeg"
  read -n1 -p "Taste drücken …"; exit 1
fi
if [ ! -d .venv ]; then
  python3 -m venv .venv
  .venv/bin/pip install --upgrade pip
  .venv/bin/pip install -r requirements.txt
fi
.venv/bin/python serve.py
