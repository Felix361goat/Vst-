from .router import router, startup

MODULE = {
    "id": "money",
    "name": "Angebote & Rechnungen",
    "icon": "🧾",
    "order": 30,
    "description": "Angebote, Rechnungen mit Zahlungs-QR, Einnahmen & Ausgaben",
    "router": router,
    "page": "page.js",
    "schema": "schema.sql",
    "startup": startup,
    "settings": [
        {"key": "BIZ_NAME", "label": "Firmen-/Künstlername auf Rechnungen", "secret": False, "help": "z.B. Felix Muster – Video & Content"},
        {"key": "BIZ_ADDRESS", "label": "Deine Anschrift", "secret": False, "help": "Straße Nr, PLZ Ort (Pflichtangabe auf Rechnungen)"},
        {"key": "BIZ_EMAIL", "label": "E-Mail auf Rechnungen", "secret": False, "help": ""},
        {"key": "IBAN", "label": "IBAN", "secret": False, "help": "für den Zahlungs-QR-Code"},
        {"key": "BIC", "label": "BIC (optional)", "secret": False, "help": ""},
        {"key": "UID", "label": "UID-Nummer (falls vorhanden)", "secret": False, "help": "Als Kleinunternehmer meist leer"},
        {"key": "KLEINUNTERNEHMER", "label": "Kleinunternehmer (ja/nein)", "secret": False,
         "help": "ja = keine USt., Hinweis §6 Abs. 1 Z 27 UStG auf der Rechnung. Grenze 55.000 € brutto/Jahr."},
        {"key": "PAYMENT_DAYS", "label": "Zahlungsziel (Tage)", "secret": False, "help": "Standard 14"},
    ],
}
