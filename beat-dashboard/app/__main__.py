import uvicorn

from .config import settings


def lan_enabled() -> bool:
    return (settings.get("LAN_ACCESS") or "nein").strip().lower() in ("ja", "yes", "1", "true")


if __name__ == "__main__":
    host = "0.0.0.0" if lan_enabled() and settings.get("ACCESS_PIN") else "127.0.0.1"
    uvicorn.run("app.main:app", host=host, port=int(settings.get("PORT") or 8765))
