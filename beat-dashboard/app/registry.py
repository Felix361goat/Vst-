"""Module discovery.

A module is a package in app/modules/<id>/ whose __init__.py exposes:

    MODULE = {
        "id": "seo", "name": "YouTube SEO", "icon": "🎯", "order": 10,
        "router": router,            # fastapi.APIRouter -> mounted at /api/<id>
        "page": "page.js",           # ES module in the package folder, served at /modules/<id>/page.js
        "schema": "schema.sql",      # optional, run at startup (use CREATE ... IF NOT EXISTS)
        "settings": [...],           # optional, extra .env keys shown on the settings page
        "description": "...",
    }

Nothing in the core has to change to add a module.
"""
from __future__ import annotations

import importlib
import logging
import pkgutil
from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse

from . import db, modules as modules_pkg

log = logging.getLogger("beat-dashboard")
LOADED: list[dict] = []
ALLOWED_STATIC = {".js", ".css", ".html", ".svg", ".png", ".json"}


def discover(app: FastAPI) -> None:
    LOADED.clear()
    for info in pkgutil.iter_modules(modules_pkg.__path__):
        if not info.ispkg:
            continue
        try:
            pkg = importlib.import_module(f"{modules_pkg.__name__}.{info.name}")
            mod = dict(pkg.MODULE)
        except Exception:  # a broken module must not take the dashboard down
            log.exception("Modul %s konnte nicht geladen werden", info.name)
            continue
        folder = Path(pkg.__file__).parent
        mod["_folder"] = folder
        if mod.get("schema"):
            db.run_module_schema(folder / mod["schema"])
        if mod.get("router") is not None:
            app.include_router(mod["router"], prefix=f"/api/{mod['id']}", tags=[mod["id"]])
        LOADED.append(mod)
    LOADED.sort(key=lambda m: m.get("order", 100))

    @app.get("/modules/{mod_id}/{path:path}", include_in_schema=False)
    def module_static(mod_id: str, path: str):
        mod = next((m for m in LOADED if m["id"] == mod_id), None)
        if not mod:
            raise HTTPException(404)
        folder: Path = mod["_folder"]
        target = (folder / path).resolve()
        if folder.resolve() not in target.parents or target.suffix not in ALLOWED_STATIC or not target.is_file():
            raise HTTPException(404)
        return FileResponse(target, headers={"Cache-Control": "no-cache"})


def enabled_state() -> dict[str, bool]:
    with db.tx() as c:
        rows = c.execute("SELECT id, enabled FROM module_state").fetchall()
    return {r["id"]: bool(r["enabled"]) for r in rows}


def public_list() -> list[dict]:
    state = enabled_state()
    return [
        {
            "id": m["id"],
            "name": m.get("name", m["id"]),
            "icon": m.get("icon", "•"),
            "description": m.get("description", ""),
            "page": f"/modules/{m['id']}/{m['page']}" if m.get("page") else None,
            "enabled": state.get(m["id"], m.get("enabled", True)),
            "core": m.get("core", False),
        }
        for m in LOADED
    ]


def module_settings() -> list[dict]:
    out = []
    for m in LOADED:
        for s in m.get("settings", []):
            out.append({**s, "module": m["id"]})
    return out
