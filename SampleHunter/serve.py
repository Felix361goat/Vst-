#!/usr/bin/env python3
"""The sample library in your browser.

    python serve.py            # opens http://localhost:8765

Listen, filter, like/dislike, download single WAVs or all your likes as a ZIP,
and start a new hunt from the page.
"""

from __future__ import annotations

import io
import json
import re
import os
import subprocess
import sys
import threading
import urllib.parse
import webbrowser
import zipfile
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from hunter.library import Library  # noqa: E402

LIB = Library(os.environ.get("HUNTER_LIBRARY", os.path.join(HERE, "library")))
LOG = os.path.join(LIB.root, "hunt.log")
_hunt: dict = {"proc": None, "auto": None}
_hunt_lock = threading.Lock()


def hunting() -> bool:
    p = _hunt["proc"]
    return p is not None and p.poll() is None


def start_hunt(body: dict, append: bool = False) -> bool:
    with _hunt_lock:
        if hunting():
            return False
        cmd = [sys.executable, os.path.join(HERE, "hunt.py"),
               "--count", str(max(1, min(500, int(body.get("count", 10))))), "--library", LIB.root]
        terms = str(body.get("terms") or "").strip()
        if terms:
            cmd += ["--terms", terms]
        for key, flag in (("allow_nc", "--allow-nc"), ("include_claimed", "--include-claimed"),
                          ("experiment", "--experiment")):
            if body.get(key):
                cmd.append(flag)
        log = open(LOG, "a" if append else "w", encoding="utf-8")
        env = dict(os.environ, PYTHONIOENCODING="utf-8", PYTHONUNBUFFERED="1")
        _hunt["proc"] = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT, env=env)
        return True


def autopilot() -> None:
    """Keeps digging while the app is open: a new round shortly after each one ends."""
    import time
    while True:
        time.sleep(5)
        params = _hunt.get("auto")
        if params and not hunting():
            time.sleep(10)
            if _hunt.get("auto") and not hunting():
                start_hunt(params, append=True)


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=os.path.join(HERE, "web"), **kw)

    def log_message(self, *a):
        pass

    def translate_path(self, path):
        clean = urllib.parse.urlparse(path).path
        if clean.startswith("/library/"):
            rel = os.path.normpath(urllib.parse.unquote(clean[len("/library/"):]))
            if rel.startswith("..") or os.path.isabs(rel):
                return os.path.join(HERE, "web", "404")
            return os.path.join(LIB.root, rel)
        return super().translate_path(path)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def _json(self, obj, code=200):
        body = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _body(self) -> dict:
        n = int(self.headers.get("Content-Length") or 0)
        try:
            return json.loads(self.rfile.read(n) or b"{}")
        except json.JSONDecodeError:
            return {}

    def do_GET(self):
        url = urllib.parse.urlparse(self.path)
        if url.path == "/api/library":
            data = LIB.load()
            return self._json({"samples": data["samples"], "ratings": LIB.ratings(),
                               "collections": LIB.collections(), "hunting": hunting(),
                               "auto": bool(_hunt.get("auto"))})
        if url.path == "/api/log":
            try:
                with open(LOG, encoding="utf-8", errors="replace") as fh:
                    lines = fh.read().splitlines()[-40:]
            except FileNotFoundError:
                lines = []
            return self._json({"hunting": hunting(), "auto": bool(_hunt.get("auto")), "lines": lines})
        if url.path == "/api/zip":
            q = urllib.parse.parse_qs(url.query)
            ids = set((q.get("ids") or [""])[0].split(","))
            data = LIB.load()
            buf = io.BytesIO()
            with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED) as z:
                credits = []
                for s in data["samples"]:
                    if s["id"] in ids:
                        z.write(os.path.join(LIB.root, s["file"]), os.path.basename(s["file"]))
                        credits.append("%s\n  %s%s (%s)\n  Quelle: %s\n  Lizenz: %s %s\n" % (
                            os.path.basename(s["file"]), s["title"],
                            (" – " + s["creator"]) if s.get("creator") else "",
                            s.get("year") or "o. J.", s["source_url"], s["license_label"],
                            s.get("license_url") or ""))
                z.writestr("CREDITS.txt", "\n".join(credits))
            body = buf.getvalue()
            self.send_response(200)
            self.send_header("Content-Type", "application/zip")
            fname = re.sub(r"[^A-Za-z0-9 _-]+", "", (q.get("name") or ["samplehunter"])[0]).strip() or "samplehunter"
            self.send_header("Content-Disposition", 'attachment; filename="%s.zip"' % fname)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        return super().do_GET()

    def do_POST(self):
        url = urllib.parse.urlparse(self.path)
        body = self._body()
        if url.path == "/api/rate":
            LIB.rate(str(body.get("id")), int(body.get("value", 0)))
            return self._json({"ok": True})
        if url.path == "/api/delete":
            LIB.delete(str(body.get("id")))
            return self._json({"ok": True})
        if url.path == "/api/collect":
            LIB.collect(str(body.get("id") or ""), str(body.get("name") or ""), bool(body.get("on", True)))
            return self._json({"ok": True, "collections": LIB.collections()})
        if url.path == "/api/collection/delete":
            LIB.drop_collection(str(body.get("name") or ""))
            return self._json({"ok": True, "collections": LIB.collections()})
        if url.path == "/api/hunt":
            _hunt["auto"] = dict(body) if body.get("auto") else None
            if not start_hunt(body):
                return self._json({"ok": False, "error": "läuft schon"}, 409)
            return self._json({"ok": True})
        if url.path == "/api/stop":
            _hunt["auto"] = None
            p = _hunt["proc"]
            if p and p.poll() is None:
                p.terminate()
            return self._json({"ok": True})
        self._json({"error": "unknown"}, 404)


def _app_browser() -> str | None:
    """A Chromium browser that can open a page as its own app window."""
    import shutil
    names = ["msedge", "chrome", "google-chrome", "chromium", "chromium-browser", "brave"]
    for n in names:
        p = shutil.which(n)
        if p:
            return p
    pf = [os.environ.get(k, "") for k in ("PROGRAMFILES(X86)", "PROGRAMFILES", "LOCALAPPDATA")]
    candidates = [os.path.join(b, *rest) for b in pf if b for rest in (
        ("Microsoft", "Edge", "Application", "msedge.exe"),
        ("Google", "Chrome", "Application", "chrome.exe"),
        ("BraveSoftware", "Brave-Browser", "Application", "brave.exe"))]
    candidates += ["/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
                   "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge",
                   "/Applications/Brave Browser.app/Contents/MacOS/Brave Browser",
                   "/Applications/Chromium.app/Contents/MacOS/Chromium"]
    for c in candidates:
        if os.path.exists(c):
            return c
    return None


def open_window(url: str, server) -> None:
    """Open SampleHunter in its own window. Closing it quits the program."""
    exe = _app_browser()
    if not exe:
        webbrowser.open(url)        # no Chromium browser: a normal tab will do
        return
    profile = os.path.join(LIB.root, ".app-window")
    proc = subprocess.Popen([exe, "--app=" + url, "--user-data-dir=" + profile,
                             "--window-size=1440,900", "--no-first-run",
                             "--no-default-browser-check", "--autoplay-policy=no-user-gesture-required"])

    def watch():
        import time
        t0 = time.time()
        proc.wait()
        if time.time() - t0 < 8:
            # The launcher handed off to another browser process and left:
            # we cannot tell when the window closes, so keep running.
            print("Fenster offen – zum Beenden dieses Fenster hier schließen oder Strg+C.")
            return
        print("Fenster geschlossen – SampleHunter beendet.")
        if hunting():
            _hunt["proc"].terminate()
        server.shutdown()
    threading.Thread(target=watch, daemon=True).start()


def main():
    port = int(os.environ.get("PORT", "8765"))
    try:
        server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    except OSError:
        # Already running: just open another window onto it.
        print("SampleHunter läuft schon – öffne das Fenster.")
        webbrowser.open("http://localhost:%d" % port)
        return
    url = "http://localhost:%d" % port
    threading.Thread(target=autopilot, daemon=True).start()
    print("SampleHunter läuft auf", url)
    print("Fenster schließen oder Strg+C zum Beenden.")
    if "--no-browser" not in sys.argv:
        if "--tab" in sys.argv:
            threading.Timer(0.8, lambda: webbrowser.open(url)).start()
        else:
            threading.Timer(0.5, lambda: open_window(url, server)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
