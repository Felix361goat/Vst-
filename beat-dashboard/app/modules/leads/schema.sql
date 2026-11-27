CREATE TABLE IF NOT EXISTS leads(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    kind TEXT NOT NULL,                     -- artist | restaurant | other
    name TEXT NOT NULL,
    instagram TEXT, email TEXT, phone TEXT, website TEXT,
    address TEXT, district TEXT, lat REAL, lon REAL,
    category TEXT,                          -- cuisine / genre
    source TEXT, source_id TEXT,
    status TEXT NOT NULL DEFAULT 'neu',
    score INTEGER DEFAULT 0,
    flags TEXT DEFAULT '{}',                -- checkboxes + qualifiers (JSON)
    extra TEXT DEFAULT '{}',                -- raw extra data (JSON)
    followers INTEGER, monthly_listeners INTEGER,
    tone TEXT DEFAULT 'du',
    notes TEXT DEFAULT '',
    deal_value_est REAL, deal_value_final REAL, lost_reason TEXT,
    next_action_date TEXT,
    first_contact_at TEXT, last_contact_at TEXT, replied_at TEXT,
    contact_count INTEGER DEFAULT 0,
    starred INTEGER DEFAULT 0,
    archived INTEGER DEFAULT 0,
    created_at TEXT DEFAULT (datetime('now','localtime')),
    updated_at TEXT DEFAULT (datetime('now','localtime'))
);
CREATE UNIQUE INDEX IF NOT EXISTS leads_source ON leads(kind, source, source_id);
CREATE INDEX IF NOT EXISTS leads_status ON leads(kind, status);
CREATE INDEX IF NOT EXISTS leads_ig ON leads(instagram);

CREATE TABLE IF NOT EXISTS lead_activities(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    lead_id INTEGER NOT NULL REFERENCES leads(id) ON DELETE CASCADE,
    ts TEXT DEFAULT (datetime('now','localtime')),
    type TEXT,          -- contact | reply | status | note | call | visit
    channel TEXT,       -- instagram | email | phone | visit | form
    text TEXT
);
CREATE INDEX IF NOT EXISTS act_lead ON lead_activities(lead_id);

CREATE TABLE IF NOT EXISTS lead_templates(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    kind TEXT, channel TEXT, name TEXT, subject TEXT, body TEXT,
    sort INTEGER DEFAULT 0
);
