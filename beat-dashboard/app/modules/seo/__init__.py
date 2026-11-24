from .router import router

MODULE = {
    "id": "seo",
    "name": "YouTube SEO",
    "icon": "🎯",
    "order": 10,
    "description": "Type-Beat-Analyse: Keywords, Konkurrenz, Titel, Tags, Beschreibung",
    "router": router,
    "page": "page.js",
    "schema": "schema.sql",
    "settings": [
        {"key": "MY_SUBSCRIBERS", "label": "Deine aktuellen Abonnenten", "secret": False,
         "help": "Für realistische Empfehlungen (kleine Kanäle sollten große Namen meiden)"},
        {"key": "MY_CHANNEL_ID", "label": "Deine YouTube-Kanal-ID (optional)", "secret": False,
         "help": "Beginnt mit UC… - dann holt das Dashboard deine Abozahl automatisch"},
    ],
}
