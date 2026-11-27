CREATE TABLE IF NOT EXISTS portfolio_items(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    niche TEXT NOT NULL,                 -- musikvideo | food
    title TEXT, client TEXT, url TEXT, image TEXT, result TEXT,
    sort INTEGER DEFAULT 0, visible INTEGER DEFAULT 1,
    created_at TEXT DEFAULT (datetime('now','localtime'))
);
