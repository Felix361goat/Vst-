import uvicorn

from .config import settings

if __name__ == "__main__":
    uvicorn.run("app.main:app", host="127.0.0.1", port=int(settings.get("PORT") or 8765))
