@echo off
rem SampleHunter starten: installiert beim ersten Mal die Python-Pakete
rem und oeffnet die Bibliothek im Browser.
cd /d "%~dp0"
where ffmpeg >nul 2>nul || (
  echo ffmpeg fehlt. Einmal ausfuehren:  winget install Gyan.FFmpeg
  echo Danach dieses Fenster schliessen und neu starten.
  pause
  exit /b 1
)
if not exist .venv (
  py -3 -m venv .venv || python -m venv .venv
  .venv\Scripts\python -m pip install --upgrade pip
  .venv\Scripts\python -m pip install -r requirements.txt
)
.venv\Scripts\python serve.py
