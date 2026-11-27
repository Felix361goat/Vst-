CREATE TABLE IF NOT EXISTS posts(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date TEXT NOT NULL, time TEXT DEFAULT '',
    platform TEXT NOT NULL,           -- yt_beat | yt_short | ig_reel | ig_post | tiktok
    title TEXT, status TEXT DEFAULT 'idee',   -- idee | produziert | geplant | gepostet
    link TEXT, notes TEXT, analysis_id INTEGER, shoot_id INTEGER,
    created_at TEXT DEFAULT (datetime('now','localtime'))
);
CREATE INDEX IF NOT EXISTS posts_date ON posts(date);
