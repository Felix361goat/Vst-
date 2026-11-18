"""The app updates itself from GitHub, so nobody downloads a ZIP twice.

The app version sits in VERSION (first line: the version, second: what is
new). The app reads the VERSION on GitHub; when it differs, an update button
appears. Updating downloads the branch as a ZIP and writes the
SampleHunter folder over the installed one, leaving the Python environment
(.venv) and your library (library/) alone, then restarts the app.
"""

from __future__ import annotations

import io
import os
import sys
import urllib.request
import zipfile

REPO = os.environ.get("HUNTER_REPO", "felix361goat/vst-")
BRANCH = os.environ.get("HUNTER_BRANCH", "ccr-3b17585f-r0bt64")
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION_FILE = os.path.join(HERE, "VERSION")
KEEP = (".venv", "library")          # never touched by an update


RAW = "https://raw.githubusercontent.com/%s/%s/SampleHunter/" % (REPO, BRANCH)


def current() -> str | None:
    try:
        with open(VERSION_FILE, encoding="utf-8") as fh:
            return fh.read().strip() or None
    except OSError:
        return None


def latest() -> dict:
    """The app version on GitHub. Catalogue pushes do not change it, so the
    update button only shows for a new app, not for new samples."""
    req = urllib.request.Request(RAW + "VERSION", headers={"User-Agent": "SampleHunter"})
    with urllib.request.urlopen(req, timeout=20) as r:
        lines = r.read().decode().strip().splitlines()
    return {"version": lines[0].strip(), "message": lines[1].strip() if len(lines) > 1 else ""}


def check() -> dict:
    try:
        new = latest()
    except Exception as exc:
        return {"available": False, "error": str(exc)}
    cur = current()
    cur = cur.splitlines()[0].strip() if cur else None
    return {"available": cur != new["version"], "current": cur, "latest": new["version"],
            "message": new["message"]}


def apply() -> str:
    """Download the newest version and write it over this installation
    (VERSION comes along with it)."""
    url = "https://codeload.github.com/%s/zip/refs/heads/%s" % (REPO, BRANCH)
    req = urllib.request.Request(url, headers={"User-Agent": "SampleHunter"})
    with urllib.request.urlopen(req, timeout=120) as r:
        body = r.read()
    with zipfile.ZipFile(io.BytesIO(body)) as z:
        prefix = None
        for name in z.namelist():
            parts = name.split("/")
            if len(parts) > 2 and parts[1] == "SampleHunter":
                prefix = parts[0] + "/SampleHunter/"
                break
        if not prefix:
            raise IOError("SampleHunter nicht im Download gefunden")
        for info in z.infolist():
            if not info.filename.startswith(prefix) or info.is_dir():
                continue
            rel = info.filename[len(prefix):]
            if not rel or rel.split("/")[0] in KEEP or ".." in rel.split("/"):
                continue
            dest = os.path.join(HERE, *rel.split("/"))
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            tmp = dest + ".new"
            with z.open(info) as src, open(tmp, "wb") as out:
                out.write(src.read())
            os.replace(tmp, dest)
    # A new version may need a new package; already-installed ones are skipped fast.
    import subprocess
    flags = {"creationflags": 0x08000000} if os.name == "nt" else {}
    for req in ("requirements.txt", "requirements-app.txt"):
        subprocess.run([sys.executable, "-m", "pip", "install", "-q", "-r", os.path.join(HERE, req)],
                       capture_output=True, **flags)
    return current() or ""


def restart() -> None:
    """Start this program again (after a short pause so the old one can let
    go of its port) and leave."""
    import subprocess
    env = dict(os.environ, HUNTER_WAIT="2")
    flags = {"creationflags": 0x00000008 | 0x00000200} if os.name == "nt" else {"start_new_session": True}
    subprocess.Popen([sys.executable] + sys.argv, cwd=HERE, env=env, **flags)
    os._exit(0)
