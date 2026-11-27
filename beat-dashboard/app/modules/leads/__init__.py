from .router import router, startup

MODULE = {
    "id": "leads",
    "name": "Leads & CRM",
    "icon": "🤝",
    "order": 20,
    "description": "Rapper & Restaurants finden, anschreiben, Antworten tracken",
    "router": router,
    "page": "page.js",
    "schema": "schema.sql",
    "startup": startup,
    "settings": [
        {"key": "MY_NAME", "label": "Dein Name (für Nachrichten)", "secret": False, "help": "z.B. Felix"},
        {"key": "MY_BEZIRK", "label": "Dein Ort / Bezirk", "secret": False, "help": "z.B. Mödling"},
        {"key": "MY_PHONE", "label": "Deine Telefonnummer", "secret": False, "help": "für E-Mail-Signatur"},
        {"key": "PORTFOLIO_LINK", "label": "Portfolio-/Showreel-Link", "secret": False,
         "help": "Instagram, Website oder YouTube-Showreel"},
    ],
}
