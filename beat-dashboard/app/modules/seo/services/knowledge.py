"""Type-beat know-how as data. Edit freely - this is the dashboard's 'brain'.

Artist lists are a starting point (Nov 2026). The dashboard checks live demand
via autocomplete/YouTube, so stale names simply score low.
"""
from __future__ import annotations

from datetime import date

GENRES: dict[str, dict] = {
    "trap": {
        "label": "Trap / Melodic Trap", "bpm": (130, 160), "sweet": (140, 150), "year_in_title": True,
        "keywords": ["trap", "dark", "hard", "melodic", "pain", "808", "bells", "flute", "guitar"],
        "artists": ["Future", "Lil Baby", "Gunna", "Travis Scott", "Don Toliver", "21 Savage", "Metro Boomin",
                    "Young Thug", "Lil Durk", "NBA YoungBoy", "Rod Wave", "Hunxho", "Lil Tecca", "Nocap", "BigXthaPlug"],
    },
    "rage": {
        "label": "Rage", "bpm": (150, 170), "sweet": (155, 165), "year_in_title": True,
        "keywords": ["rage", "distorted synth", "hyper", "mosh", "evil", "opium"],
        "artists": ["Ken Carson", "Destroy Lonely", "Playboi Carti", "Yeat", "Homixide Gang", "Lil Uzi Vert",
                    "OsamaSon", "Che", "Trippie Redd"],
    },
    "plugg": {
        "label": "Plugg / Pluggnb", "bpm": (140, 160), "sweet": (145, 155), "year_in_title": True,
        "keywords": ["plugg", "pluggnb", "bouncy", "ethereal", "beat skip", "2 step"],
        "artists": ["Summrs", "Autumn!", "Kankan", "Lucki", "Tana", "Yung Fazo", "Nettspend", "Xaviersobased", "Babyxsosa"],
    },
    "jerk": {
        "label": "Jerk", "bpm": (140, 160), "sweet": (145, 155), "year_in_title": True,
        "keywords": ["jerk", "digicore", "underground", "glitchy"],
        "artists": ["Xaviersobased", "Nettspend", "Kashpaint", "fakemink", "OsamaSon", "Che", "2hollis"],
    },
    "hyperpop": {
        "label": "Hyperpop / Digicore", "bpm": (140, 180), "sweet": (150, 165), "year_in_title": True,
        "keywords": ["hyperpop", "digicore", "glitch", "emo"],
        "artists": ["2hollis", "glaive", "ericdoa", "Jane Remover", "Bladee", "Yung Lean", "Midwxst"],
    },
    "uk_drill": {
        "label": "UK Drill", "bpm": (138, 145), "sweet": (140, 142), "year_in_title": True,
        "keywords": ["uk drill", "sliding 808", "dark piano", "choir", "strings"],
        "artists": ["Central Cee", "Digga D", "Headie One", "K-Trap", "Fredo", "Unknown T", "Kwengface"],
    },
    "ny_drill": {
        "label": "NY / Bronx / Sample Drill", "bpm": (140, 150), "sweet": (142, 146), "year_in_title": True,
        "keywords": ["ny drill", "bronx drill", "sample drill", "sexy drill"],
        "artists": ["Kyle Richh", "Jenn Carter", "Sha EK", "Dee Billz", "Fivio Foreign", "Cash Cobain", "Pop Smoke"],
    },
    "jersey_club": {
        "label": "Jersey Club", "bpm": (130, 145), "sweet": (140, 140), "year_in_title": True,
        "keywords": ["jersey club", "bed squeak", "triplet kick", "sped up"],
        "artists": ["Bandmanrill", "Lil Uzi Vert", "Ice Spice", "Cash Cobain"],
    },
    "boom_bap": {
        "label": "Boom Bap", "bpm": (85, 95), "sweet": (88, 92), "year_in_title": False,
        "keywords": ["boom bap", "old school", "90s", "dusty", "sample", "jazzy"],
        "artists": ["J. Cole", "Joey Bada$$", "Westside Gunn", "Benny the Butcher", "Conway", "Kendrick Lamar", "Larry June"],
    },
    "rnb": {
        "label": "R&B / RnB", "bpm": (60, 90), "sweet": (65, 80), "year_in_title": False,
        "keywords": ["rnb", "dark rnb", "sad", "love", "guitar", "slow jam", "2000s"],
        "artists": ["SZA", "Brent Faiyaz", "PartyNextDoor", "Bryson Tiller", "Summer Walker", "6LACK", "The Weeknd",
                    "Leon Thomas", "Giveon", "Drake"],
    },
    "afro": {
        "label": "Afrobeats / Afro RnB / Amapiano", "bpm": (95, 115), "sweet": (100, 110), "year_in_title": True,
        "keywords": ["afrobeat", "afro rnb", "afro fusion", "amapiano", "log drum", "afroswing"],
        "artists": ["Burna Boy", "Wizkid", "Rema", "Asake", "Tems", "Ayra Starr", "Omah Lay", "Tyla", "Davido"],
    },
    "lofi": {
        "label": "Lofi", "bpm": (70, 90), "sweet": (75, 85), "year_in_title": False,
        "keywords": ["lofi", "chill", "jazzy", "study", "rainy", "nostalgic"],
        "artists": ["J Dilla", "Nujabes", "Mac Miller", "Joji"],
    },
    "detroit": {
        "label": "Detroit / Flint", "bpm": (90, 105), "sweet": (95, 100), "year_in_title": True,
        "keywords": ["detroit", "flint", "detroit type beat"],
        "artists": ["BabyTron", "Babyface Ray", "Veeze", "Rio Da Yung OG"],
    },
    "memphis": {
        "label": "Memphis", "bpm": (140, 160), "sweet": (145, 155), "year_in_title": True,
        "keywords": ["memphis", "cowbell", "phonk"],
        "artists": ["GloRilla", "Key Glock", "Pooh Shiesty"],
    },
}

MOODS = ["Dark", "Hard", "Sad", "Melodic", "Emotional", "Aggressive", "Bouncy", "Ambient", "Pain", "Love",
         "Wavy", "Chill", "Spacey", "Evil", "Hype"]

FORMAT_MODIFIERS = ["Sped Up", "Slowed", "Jersey Club", "Afro Remix", "2-Step", "Guitar", "Sample"]

# Artists whose "type beat" is so saturated that a small channel won't rank
HEAD_TERMS = {"drake", "travis scott", "future", "lil baby", "21 savage", "playboi carti", "lil uzi vert",
              "nba youngboy", "j. cole", "kendrick lamar", "the weeknd", "central cee"}

# Spelling variants / misspellings for tags (never for titles)
ARTIST_VARIANTS: dict[str, list[str]] = {
    "ken carson": ["kencarson"],
    "playboi carti": ["carti", "playboy carti"],
    "lil uzi vert": ["lil uzi", "uzi vert"],
    "j. cole": ["j cole"],
    "autumn!": ["autumn"],
    "21 savage": ["21savage"],
    "nba youngboy": ["youngboy", "nba young boy"],
    "travis scott": ["travis"],
    "don toliver": ["don tolliver"],
    "partynextdoor": ["party next door", "pnd"],
    "joey bada$$": ["joey badass"],
    "central cee": ["cench", "central c"],
}

# Words that make metadata misleading (YouTube spam/deceptive practices policy)
BANNED_TITLE_WORDS = ["ft.", "feat", "leak", "leaked", "unreleased", "official", "snippet", "remix of"]

KEY_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def title_year() -> int:
    """After Nov 1, producers already title with next year."""
    today = date.today()
    return today.year + 1 if today.month >= 11 else today.year


def genre_for_bpm(bpm: float) -> list[str]:
    hits = []
    for gid, g in GENRES.items():
        lo, hi = g["bpm"]
        if lo <= bpm <= hi or lo <= bpm * 2 <= hi or lo <= bpm / 2 <= hi:
            hits.append(gid)
    return hits


def bpm_fit(genre: str, bpm: float | None) -> dict | None:
    g = GENRES.get(genre)
    if not g or not bpm:
        return None
    lo, hi = g["bpm"]
    for factor, label in ((1, ""), (2, " (doppelt gezählt)"), (0.5, " (halb gezählt)")):
        b = bpm * factor
        if lo <= b <= hi:
            return {"ok": True, "message": f"{round(b)} BPM passt zu {g['label']} ({lo}–{hi}){label}"}
    return {"ok": False, "message": f"{round(bpm)} BPM ist untypisch für {g['label']} (üblich {lo}–{hi} BPM)"}
