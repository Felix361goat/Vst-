from .router import router

MODULE = {
    "id": "content",
    "name": "Content-Plan",
    "icon": "📅",
    "order": 15,
    "description": "Kalender für Beats, Shorts, Reels & TikToks – mit Wochenzielen",
    "router": router,
    "page": "page.js",
    "schema": "schema.sql",
    "settings": [
        {"key": "GOAL_BEATS_WEEK", "label": "Ziel: Beats pro Woche", "secret": False, "help": "Empfehlung: mind. 3"},
        {"key": "GOAL_SHORTS_WEEK", "label": "Ziel: Shorts/Reels/TikToks pro Woche", "secret": False, "help": "Empfehlung: 1–2 pro Beat + eigene Videografie-Reels"},
    ],
}
