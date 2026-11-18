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

from hunter.cloud import Store  # noqa: E402
from hunter.library import Library  # noqa: E402

LIB = Library(os.environ.get("HUNTER_LIBRARY", os.path.join(HERE, "library")))
LOG = os.path.join(LIB.root, "hunt.log")
STORE = Store(LIB.root)


def audio_path(sample: dict) -> str:
    """A playable file for any sample: its own file, or fetched from the cloud."""
    if sample.get("file") and os.path.exists(os.path.join(LIB.root, sample["file"])):
        return os.path.join(LIB.root, sample["file"])
    return STORE.preview(sample)


def full_path(sample: dict) -> str:
    """The best-quality copy, kept on disk for good (for drags and downloads)."""
    if sample.get("file") and os.path.exists(os.path.join(LIB.root, sample["file"])):
        return os.path.join(LIB.root, sample["file"])
    return STORE.keep(sample)
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


EXPORT_DIR = os.environ.get("HUNTER_EXPORT_DIR") or os.path.join(os.path.expanduser("~"), "Music", "SampleHunter")


def export_path(sample_id: str, fmt: str) -> str | None:
    """The sample as a named file in the wanted format, in Music/SampleHunter.

    Whatever you drag into FL Studio or download lands there and stays there:
    a project refers to the file by its path, so it must never move."""
    s = LIB.find(sample_id)
    if not s:
        return None
    src = full_path(s)
    fmt = "wav" if fmt == "wav" else "mp3"
    os.makedirs(EXPORT_DIR, exist_ok=True)
    dest = os.path.join(EXPORT_DIR, s["name"] + "." + fmt)
    if not os.path.exists(dest):
        from hunter import analyze
        if fmt == "wav":
            analyze.to_wav(src, dest, s.get("duration"))
        elif src.lower().endswith(".mp3"):
            import shutil
            shutil.copyfile(src, dest)
        else:
            analyze.write_audio(dest, analyze.decode(src, sr=44100, channels=2, duration=None), 44100)
    return dest


def _safe(fn, *a):
    try:
        fn(*a)
    except Exception as exc:  # a vanished source must not take the server down
        print("Hinweis:", exc)


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
            return self._json({"samples": LIB.all_samples(), "ratings": LIB.ratings(), "usage": STORE.usage(),
                               "collections": LIB.collections(), "hunting": hunting(),
                               "auto": bool(_hunt.get("auto"))})
        if url.path == "/api/log":
            try:
                with open(LOG, encoding="utf-8", errors="replace") as fh:
                    lines = fh.read().splitlines()[-40:]
            except FileNotFoundError:
                lines = []
            return self._json({"hunting": hunting(), "auto": bool(_hunt.get("auto")), "lines": lines})
        if url.path == "/api/listen":
            # Search the library by describing a sound.
            q = (urllib.parse.parse_qs(url.query).get("q") or [""])[0].strip()
            with_emb = [x for x in LIB.all_samples() if x.get("emb")]
            if not q or not with_emb:
                return self._json({"ids": [], "error": None if q else "leer"})
            try:
                from hunter import ear
                if not ear.available():
                    return self._json({"ids": [], "error": "KI-Ohr nicht installiert"})
                t = ear.text_embedding(q)
            except Exception as exc:
                return self._json({"ids": [], "error": str(exc)})
            import numpy as np
            E = np.array([x["emb"] for x in with_emb])
            order = np.argsort(-(E @ t))
            return self._json({"ids": [with_emb[i]["id"] for i in order[:60]]})
        if url.path == "/api/prepare":
            # A press on a sample may become a drag: start fetching it now.
            q = urllib.parse.parse_qs(url.query)
            sid, fmt = (q.get("id") or [""])[0], (q.get("fmt") or ["mp3"])[0]
            threading.Thread(target=lambda: _safe(export_path, sid, fmt), daemon=True).start()
            return self._json({"ok": True})
        if url.path == "/api/audio":
            s = LIB.find((urllib.parse.parse_qs(url.query).get("id") or [""])[0])
            if not s:
                return self._json({"error": "unbekannt"}, 404)
            try:
                path = audio_path(s)
            except Exception as exc:
                return self._json({"error": "Quelle nicht erreichbar: %s" % exc}, 502)
            with open(path, "rb") as fh:
                body = fh.read()
            self.send_response(200)
            self.send_header("Content-Type", "audio/wav" if path.endswith(".wav") else "audio/mpeg")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if url.path == "/api/file":
            q = urllib.parse.parse_qs(url.query)
            path = export_path((q.get("id") or [""])[0], (q.get("fmt") or ["mp3"])[0])
            if not path:
                return self._json({"error": "unbekannt"}, 404)
            with open(path, "rb") as fh:
                body = fh.read()
            self.send_response(200)
            self.send_header("Content-Type", "audio/wav" if path.endswith(".wav") else "audio/mpeg")
            self.send_header("Content-Disposition", 'attachment; filename="%s"' % os.path.basename(path))
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if url.path == "/api/zip":
            q = urllib.parse.parse_qs(url.query)
            ids = set((q.get("ids") or [""])[0].split(","))
            data = {"samples": LIB.all_samples()}
            buf = io.BytesIO()
            with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED) as z:
                credits = []
                fmt = (q.get("fmt") or ["mp3"])[0]
                for s in data["samples"]:
                    if s["id"] in ids:
                        path = export_path(s["id"], fmt)
                        z.write(path, os.path.basename(path))
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
            sid, value = str(body.get("id")), int(body.get("value", 0))
            LIB.rate(sid, value)
            s = LIB.find(sid)
            if s and not s.get("file"):
                # Favourites are backed up locally, in case the source vanishes.
                if value > 0:
                    threading.Thread(target=lambda: _safe(STORE.keep, s), daemon=True).start()
                else:
                    STORE.release(s)
            return self._json({"ok": True})
        if url.path == "/api/decline":
            LIB.decline(str(body.get("id")))
            return self._json({"ok": True, "usage": STORE.usage()})
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
