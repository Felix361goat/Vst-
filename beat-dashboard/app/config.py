"""Settings stored in beat-dashboard/.env. Secrets never leave this machine."""
from __future__ import annotations

import os
from pathlib import Path

from dotenv import dotenv_values

ROOT = Path(__file__).resolve().parent.parent
ENV_FILE = ROOT / ".env"
DATA_DIR = ROOT / "data"
UPLOAD_DIR = DATA_DIR / "uploads"

DEFAULTS = {
    "YOUTUBE_API_KEY": "",
    "YT_DAILY_BUDGET": "8000",
    "YT_SEARCH_BUDGET": "90",
    "PRODUCER_NAME": "",
    "STORE_LINK": "",
    "INSTAGRAM": "",
    "LASTFM_API_KEY": "",
    "ANTHROPIC_API_KEY": "",
    "ANTHROPIC_MODEL": "claude-sonnet-5-5",
    "REGION_CODE": "US",
    "PORT": "8765",
}

SECRET_KEYS = {"YOUTUBE_API_KEY", "LASTFM_API_KEY", "ANTHROPIC_API_KEY"}


class Settings:
    def __init__(self) -> None:
        self._values: dict[str, str] = {}
        self.reload()

    def reload(self) -> None:
        values = dict(DEFAULTS)
        if ENV_FILE.exists():
            values.update({k: (v or "") for k, v in dotenv_values(ENV_FILE).items()})
        # real environment variables win (handy for testing)
        for k in list(values):
            if os.environ.get(k):
                values[k] = os.environ[k]
        self._values = values

    def get(self, key: str, default: str = "") -> str:
        return self._values.get(key, default) or default

    def has(self, key: str) -> bool:
        return bool(self.get(key).strip())

    def all(self) -> dict[str, str]:
        return dict(self._values)

    def save(self, updates: dict[str, str]) -> None:
        """Merge updates into .env, keeping comments and order of existing lines."""
        lines = ENV_FILE.read_text(encoding="utf-8").splitlines() if ENV_FILE.exists() else []
        pending = {k: str(v).strip() for k, v in updates.items()}
        out = []
        for line in lines:
            key = line.split("=", 1)[0].strip() if "=" in line and not line.lstrip().startswith("#") else None
            if key in pending:
                out.append(f"{key}={pending.pop(key)}")
            else:
                out.append(line)
        out.extend(f"{k}={v}" for k, v in pending.items())
        ENV_FILE.write_text("\n".join(out) + "\n", encoding="utf-8")
        self.reload()


def mask(value: str) -> str:
    if not value:
        return ""
    return "••••" + value[-4:] if len(value) > 6 else "••••"


settings = Settings()
DATA_DIR.mkdir(exist_ok=True)
UPLOAD_DIR.mkdir(exist_ok=True)
