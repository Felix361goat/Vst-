@echo off
rem SampleHunter starten. Beim ersten Mal werden ca. 300 MB installiert
rem (App-Fenster und Basis); danach startet es in Sekunden.
cd /d "%~dp0"
where py >nul 2>nul || where python >nul 2>nul || (
  echo Python fehlt. Einmal ausfuehren:  winget install Python.Python.3.12
  echo Danach dieses Fenster schliessen und neu starten.
  pause
  exit /b 1
)
if not exist .venv (
  echo Erster Start: installiere SampleHunter ^(ca. 300 MB, ein paar Minuten^) ...
  py -3 -m venv .venv 2>nul || python -m venv .venv
  .venv\Scripts\python -m pip install --upgrade pip -q
)
if not exist .venv\base-ok-2 (
  .venv\Scripts\python -m pip install -r requirements.txt || (pause & exit /b 1)
  echo ok> .venv\base-ok-2
)
if not exist .venv\app-ok (
  .venv\Scripts\python -m pip install -r requirements-app.txt && echo ok> .venv\app-ok
)
if exist .venv\app-ok (
  .venv\Scripts\python app.py
) else (
  echo App-Fenster nicht verfuegbar - starte im Browser ^(ohne Drag ^& Drop nach FL Studio^).
  .venv\Scripts\python serve.py
)
