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


_FAILS: dict[str, list[float]] = {}   # ip -> timestamps of failed PIN attempts
MAX_FAILS, LOCK_SECONDS = 5, 15 * 60


def _locked(ip: str) -> bool:
    import time
    now = time.time()
    recent = [t for t in _FAILS.get(ip, []) if now - t < LOCK_SECONDS]
    _FAILS[ip] = recent
    return len(recent) >= MAX_FAILS


def pin_ok(pin: str | None) -> bool:
    return bool(pin) and len(pin) >= 6


def _allowed_hosts() -> set[str]:
    return {"127.0.0.1", "localhost", "::1", "[::1]", "testserver", local_ip()}


def _host_of(value: str) -> str:
    """'http://192.168.1.5:8765/x' -> '192.168.1.5', 'localhost:8765' -> 'localhost', '[::1]:8765' -> '[::1]'."""
    from urllib.parse import urlsplit
    netloc = urlsplit(value).netloc if "://" in value else value
    if netloc.startswith("["):
        return netloc.split("]")[0] + "]"
    return netloc.split(":")[0]


def _is_local(request: Request) -> bool:
    host = request.client.host if request.client else ""
    return host in ("127.0.0.1", "::1", "localhost", "testclient")


async def middleware(request: Request, call_next):
    # 1) DNS-rebinding guard: only answer to our own host names
    host = _host_of(request.headers.get("host", ""))
    if host and host not in _allowed_hosts():
        return JSONResponse({"detail": "Unbekannter Host"}, status_code=403)
    # 2) CSRF guard: state-changing requests must come from the dashboard itself, not from another website
    if request.method in ("POST", "PUT", "PATCH", "DELETE"):
        origin = request.headers.get("origin") or request.headers.get("referer") or ""
        if request.headers.get("sec-fetch-site") == "cross-site" or (origin and _host_of(origin) not in _allowed_hosts()):
            return JSONResponse({"detail": "Anfrage von fremder Website blockiert"}, status_code=403)
    if _is_local(request) or request.url.path in ("/login", "/static/style.css", "/manifest.webmanifest"):
        return await call_next(request)
    pin = settings.get("ACCESS_PIN")
    if not pin_ok(pin):
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
        import time
        ip = request.client.host if request.client else "?"
        if _locked(ip):
            return HTMLResponse(LOGIN_HTML.format(err='<p class="neg small">Zu viele Fehlversuche – 15 Minuten gesperrt.</p>'), status_code=429)
        form = (await request.body()).decode()
        from urllib.parse import parse_qs
        pin = (parse_qs(form).get("pin") or [""])[0]
        real = settings.get("ACCESS_PIN")
        if pin_ok(real) and hmac.compare_digest(pin, real):
            _FAILS.pop(ip, None)
            r = RedirectResponse("/", status_code=303)
            r.set_cookie(COOKIE, _token(real), httponly=True, samesite="lax", max_age=30 * 24 * 3600)
            return r
        _FAILS.setdefault(ip, []).append(time.time())
        import logging
        logging.getLogger("beat-dashboard.lan").warning("Falsche PIN von %s", ip)
        return HTMLResponse(LOGIN_HTML.format(err='<p class="neg small">Falsche PIN</p>'), status_code=401)
