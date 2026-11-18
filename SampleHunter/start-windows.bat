@echo off
rem SampleHunter starten: installiert beim ersten Mal die Python-Pakete
rem und oeffnet die Bibliothek in einem eigenen Fenster.
cd /d "%~dp0"
where ffmpeg >nul 2>nul || (
  echo ffmpeg fehlt. Einmal ausfuehren:  winget install Gyan.FFmpeg
  echo Danach dieses Fenster schliessen und neu starten.
  pause
  exit /b 1
)
if not exist .venv (
  echo Erster Start: installiere Pakete, das dauert ein paar Minuten ...
  py -3 -m venv .venv || python -m venv .venv
  .venv\Scripts\python -m pip install --upgrade pip
  .venv\Scripts\python -m pip install -r requirements.txt || (pause & exit /b 1)
)
if not exist .venv\ear-ok (
  echo Installiere das KI-Ohr ^(ca. 1 GB, einmalig^) ...
  .venv\Scripts\python -m pip install -r requirements-ear.txt && echo ok> .venv\ear-ok
  if not exist .venv\ear-ok echo KI-Ohr liess sich nicht installieren - SampleHunter laeuft trotzdem, nur ohne.
)
.venv\Scripts\python serve.py
