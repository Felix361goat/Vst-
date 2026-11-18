#!/usr/bin/env python3
"""The sample library in your browser.

    python serve.py            # opens http://localhost:8765

Listen, filter, like/dislike, download single WAVs or all your likes as a ZIP,
and start a new hunt from the page.
"""

from __future__ import annotations

import io
import json
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
_hunt: dict = {"proc": None}
_hunt_lock = threading.Lock()


def hunting() -> bool:
    p = _hunt["proc"]
    return p is not None and p.poll() is None


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
                               "hunting": hunting()})
        if url.path == "/api/log":
            try:
                with open(LOG, encoding="utf-8", errors="replace") as fh:
                    lines = fh.read().splitlines()[-40:]
            except FileNotFoundError:
                lines = []
            return self._json({"hunting": hunting(), "lines": lines})
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
            self.send_header("Content-Disposition", 'attachment; filename="samplehunter.zip"')
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
        if url.path == "/api/hunt":
            with _hunt_lock:
                if hunting():
                    return self._json({"ok": False, "error": "läuft schon"}, 409)
                cmd = [sys.executable, os.path.join(HERE, "hunt.py"),
                       "--count", str(int(body.get("count", 10))), "--library", LIB.root]
                terms = str(body.get("terms") or "").strip()
                if terms:
                    cmd += ["--terms", terms]
                if body.get("allow_nc"):
                    cmd.append("--allow-nc")
                if body.get("include_claimed"):
                    cmd.append("--include-claimed")
                log = open(LOG, "w", encoding="utf-8")
                env = dict(os.environ, PYTHONIOENCODING="utf-8", PYTHONUNBUFFERED="1")
                _hunt["proc"] = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT, env=env)
            return self._json({"ok": True})
        if url.path == "/api/stop":
            p = _hunt["proc"]
            if p and p.poll() is None:
                p.terminate()
            return self._json({"ok": True})
        self._json({"error": "unknown"}, 404)


def main():
    port = int(os.environ.get("PORT", "8765"))
    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    url = "http://localhost:%d" % port
    print("SampleHunter läuft auf", url, "– Strg+C zum Beenden")
    if "--no-browser" not in sys.argv:
        threading.Timer(0.8, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
