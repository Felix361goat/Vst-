"""Optional phone/tablet access in the same Wi-Fi, protected by a PIN.

Requests from this PC (127.0.0.1) never need the PIN. Everything else needs a valid session cookie,
obtained via /login with ACCESS_PIN.
"""
from __future__ import annotations

import hashlib
import hmac
import secrets
import socket

from fastapi import Request
from fastapi.responses import HTMLResponse, JSONResponse, RedirectResponse

from .config import settings

COOKIE = "bd_session"
_SECRET = secrets.token_hex(16)  # new on every start -> phones log in again after a restart


def _token(pin: str) -> str:
    return hmac.new(_SECRET.encode(), pin.encode(), hashlib.sha256).hexdigest()


def local_ip() -> str:
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except OSError:
        return "127.0.0.1"


def _is_local(request: Request) -> bool:
    host = request.client.host if request.client else ""
    return host in ("127.0.0.1", "::1", "localhost", "testclient")


async def middleware(request: Request, call_next):
    if _is_local(request) or request.url.path in ("/login", "/static/style.css", "/manifest.webmanifest"):
        return await call_next(request)
    pin = settings.get("ACCESS_PIN")
    if not pin:
        return JSONResponse({"detail": "Zugriff von anderen Geräten ist nicht aktiviert"}, status_code=403)
    if hmac.compare_digest(request.cookies.get(COOKIE, ""), _token(pin)):
        return await call_next(request)
    if request.url.path.startswith("/api/"):
        return JSONResponse({"detail": "Bitte am Handy mit PIN anmelden"}, status_code=401)
    return RedirectResponse("/login")


LOGIN_HTML = """<!doctype html><html lang=de><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>Beat Dashboard</title><link rel=stylesheet href=/static/style.css></head>
<body style="display:grid;place-items:center;min-height:100vh;padding:20px">
<form method=post action=/login class=card style="width:min(340px,100%);text-align:center">
<h2>Beat Dashboard</h2><p class="muted small">PIN eingeben (steht in den Einstellungen auf deinem PC)</p>
<input name=pin type=password inputmode=numeric autofocus style="text-align:center;font-size:22px;letter-spacing:.3em;margin:10px 0">
<button class="btn primary" style="width:100%;justify-content:center">Anmelden</button>{err}</form></body></html>"""


def register(app):
    app.middleware("http")(middleware)

    @app.get("/login", include_in_schema=False)
    def login_page():
        return HTMLResponse(LOGIN_HTML.format(err=""))

    @app.post("/login", include_in_schema=False)
    async def login(request: Request):
        form = (await request.body()).decode()
        from urllib.parse import parse_qs
        pin = (parse_qs(form).get("pin") or [""])[0]
        real = settings.get("ACCESS_PIN")
        if real and hmac.compare_digest(pin, real):
            r = RedirectResponse("/", status_code=303)
            r.set_cookie(COOKIE, _token(real), httponly=True, samesite="lax", max_age=30 * 24 * 3600)
            return r
        return HTMLResponse(LOGIN_HTML.format(err='<p class="neg small">Falsche PIN</p>'), status_code=401)
