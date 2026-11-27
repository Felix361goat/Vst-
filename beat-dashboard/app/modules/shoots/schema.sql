CREATE TABLE IF NOT EXISTS shoots(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL, type TEXT DEFAULT 'musikvideo',
    lead_id INTEGER, document_id INTEGER,
    date TEXT, call_time TEXT, end_time TEXT,
    location TEXT, lat REAL, lon REAL,
    contact TEXT, crew TEXT,
    status TEXT DEFAULT 'geplant',          -- geplant | gedreht | schnitt | geliefert | abgesagt
    shots TEXT DEFAULT '[]', gear TEXT DEFAULT '[]', deliverables TEXT DEFAULT '[]',
    notes TEXT DEFAULT '',
    created_at TEXT DEFAULT (datetime('now','localtime'))
);
