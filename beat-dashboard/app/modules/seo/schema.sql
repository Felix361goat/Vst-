CREATE TABLE IF NOT EXISTS analyses(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    created_at TEXT DEFAULT (datetime('now','localtime')),
    beat_name TEXT, genre TEXT, mood TEXT, bpm REAL, musical_key TEXT,
    artists TEXT, audio_file TEXT, input TEXT, result TEXT,
    beat_id INTEGER  -- reserved: link to a future beat-catalog module
);
CREATE TABLE IF NOT EXISTS uploads(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    created_at TEXT DEFAULT (datetime('now','localtime')),
    analysis_id INTEGER REFERENCES analyses(id) ON DELETE SET NULL,
    youtube_video_id TEXT, title TEXT, keyword TEXT, tags TEXT, published_at TEXT, notes TEXT,
    beat_id INTEGER
);
CREATE TABLE IF NOT EXISTS upload_stats(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    upload_id INTEGER REFERENCES uploads(id) ON DELETE CASCADE,
    fetched_at TEXT DEFAULT (datetime('now','localtime')),
    views INTEGER, likes INTEGER, comments INTEGER
);
