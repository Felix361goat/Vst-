@echo off
setlocal
cd /d "%~dp0"
title Beat Dashboard

rem ---- Python finden -------------------------------------------------------
set "PY="
where py >nul 2>nul && set "PY=py -3"
if not defined PY (where python >nul 2>nul && set "PY=python")
if not defined PY (
  echo.
  echo  Python wurde nicht gefunden.
  echo  Bitte Python 3.11 oder neuer installieren: https://www.python.org/downloads/
  echo  WICHTIG: Beim Installieren "Add python.exe to PATH" anhaken.
  echo.
  pause
  exit /b 1
)

rem ---- Virtuelle Umgebung + Pakete (nur beim ersten Start / nach Updates) ---
if not exist ".venv\Scripts\python.exe" (
  echo Erster Start: richte alles ein ^(dauert 1-2 Minuten^) ...
  %PY% -m venv .venv || (echo Konnte .venv nicht anlegen & pause & exit /b 1)
)
set "VPY=.venv\Scripts\python.exe"
fc /b requirements.txt .venv\req.installed >nul 2>nul
if errorlevel 1 (
  "%VPY%" -m pip install --upgrade pip -q
  "%VPY%" -m pip install -r requirements.txt -q || (echo Paket-Installation fehlgeschlagen & pause & exit /b 1)
  copy /y requirements.txt .venv\req.installed >nul
)

if not exist ".env" copy ".env.example" ".env" >nul

rem ---- Starten -------------------------------------------------------------
set "PORT=8765"
for /f "tokens=2 delims==" %%a in ('findstr /b "PORT=" .env 2^>nul') do set "PORT=%%a"
echo.
echo  Beat Dashboard laeuft auf http://127.0.0.1:%PORT%
echo  Dieses Fenster offen lassen. Zum Beenden: Fenster schliessen oder Strg+C.
echo.
start "" "http://127.0.0.1:%PORT%"
"%VPY%" -m app
pause
