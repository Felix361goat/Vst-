CREATE TABLE IF NOT EXISTS documents(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type TEXT NOT NULL,                 -- angebot | rechnung
    number TEXT,                        -- A-2026-001 / R-2026-001 (set when finalised)
    lead_id INTEGER,                    -- link to leads module (optional)
    source_id INTEGER,                  -- invoice created from this quote
    client_name TEXT, client_address TEXT, client_email TEXT,
    title TEXT, items TEXT DEFAULT '[]', intro TEXT, notes TEXT, rights TEXT,
    issue_date TEXT, delivery TEXT, due_date TEXT, valid_until TEXT,
    status TEXT DEFAULT 'entwurf',      -- entwurf | gesendet | angenommen | abgelehnt | bezahlt | storniert
    total REAL DEFAULT 0, paid_at TEXT,
    created_at TEXT DEFAULT (datetime('now','localtime'))
);
CREATE UNIQUE INDEX IF NOT EXISTS doc_number ON documents(number) WHERE number IS NOT NULL;
CREATE TABLE IF NOT EXISTS ledger(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date TEXT NOT NULL, kind TEXT NOT NULL,   -- einnahme | ausgabe
    category TEXT, description TEXT, amount REAL NOT NULL,
    document_id INTEGER, created_at TEXT DEFAULT (datetime('now','localtime'))
);
