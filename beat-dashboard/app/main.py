"""FastAPI app: core API, auto-discovered modules, static frontend."""
from __future__ import annotations

import logging
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles

from . import cache, db, lan, net, registry
from .core_api import router as core_router

logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")
STATIC = Path(__file__).resolve().parent.parent / "static"


@asynccontextmanager
async def lifespan(app: FastAPI):
    cache.purge_expired()
    yield
    await net.close()


db.init_core()
app = FastAPI(title="Beat Dashboard", lifespan=lifespan)
lan.register(app)
app.include_router(core_router)
registry.discover(app)
app.mount("/static", StaticFiles(directory=STATIC), name="static")


@app.get("/", include_in_schema=False)
def index():
    return FileResponse(STATIC / "index.html", headers={"Cache-Control": "no-cache"})


@app.get("/manifest.webmanifest", include_in_schema=False)
def manifest():
    from fastapi.responses import JSONResponse
    return JSONResponse({"name": "Beat Dashboard", "short_name": "Dashboard", "start_url": "/", "display": "standalone",
                         "background_color": "#F5F5F7", "theme_color": "#F5F5F7",
                         "icons": [{"src": "/static/icon.svg", "sizes": "any", "type": "image/svg+xml"}]})
