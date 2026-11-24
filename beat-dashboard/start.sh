#!/usr/bin/env bash
# Beat Dashboard - Start fuer macOS / Linux
set -e
cd "$(dirname "$0")"
PY=$(command -v python3 || command -v python)
[ -z "$PY" ] && { echo "Python 3.11+ fehlt: https://www.python.org/downloads/"; exit 1; }
[ -x .venv/bin/python ] || { echo "Erster Start: richte alles ein ..."; "$PY" -m venv .venv; }
if ! cmp -s requirements.txt .venv/req.installed; then
  .venv/bin/python -m pip install -q --upgrade pip
  .venv/bin/python -m pip install -q -r requirements.txt
  cp requirements.txt .venv/req.installed
fi
[ -f .env ] || cp .env.example .env
PORT=$(grep -E '^PORT=' .env | cut -d= -f2); PORT=${PORT:-8765}
echo "Beat Dashboard: http://127.0.0.1:$PORT  (Strg+C zum Beenden)"
( sleep 2; (xdg-open "http://127.0.0.1:$PORT" || open "http://127.0.0.1:$PORT") >/dev/null 2>&1 ) &
exec .venv/bin/python -m app
