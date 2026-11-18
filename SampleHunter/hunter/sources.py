"""Where the raw material comes from, and whether it may be sampled.

Three sources, in order of how much their licence can be trusted:

1. 78rpm records on the Internet Archive (Great 78 Project, Library of Congress
   National Jukebox). Under US law (Music Modernization Act) a recording first
   published more than 100 years ago is public domain, as are compositions
   published more than 95 years ago. So anything dated before 1926 is clear on
   both counts in the US, in 2026. The tool computes the cut-off from today's
   date, so it moves forward a year every January.
2. Wikimedia Commons audio. Licences there are reviewed by the community:
   mostly CC BY-SA and public domain, with a lot of ethnographic recordings and
   instrument demos that exist nowhere else.
3. Other Internet Archive items whose uploader attached a free licence. Off by
   default: an uploader can write anything, and some write "public domain" on
   commercial records.

Anything with ND ("no derivatives") or with no licence at all is never used:
sampling is a derivative work.
"""

from __future__ import annotations

import html
import json
import random
import re
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass, field

USER_AGENT = "SampleHunter/1.0 (personal crate-digging tool)"
IA_SEARCH = "https://archive.org/advancedsearch.php"
IA_METADATA = "https://archive.org/metadata/"
IA_DOWNLOAD = "https://archive.org/download/"
COMMONS_API = "https://commons.wikimedia.org/w/api.php"

# Last year whose recordings are public domain in the US this year.
PD_CUTOFF_YEAR = time.gmtime().tm_year - 101

# Search vocabulary. The point is breadth: instruments and styles that rarely
# end up in sample packs. Each term doubles as a tag on what it finds.
INSTRUMENTS = [
    "tumbi", "sarangi", "shehnai", "santoor", "sitar", "tabla", "harmonium",
    "veena", "nadaswaram", "rubab", "dotar", "dombra", "komuz", "oud", "saz",
    "baglama", "bouzouki", "kanun", "qanun", "kemenche", "duduk", "zurna",
    "ney", "mijwiz", "kora", "ngoni", "balafon", "mbira", "kalimba", "likembe",
    "marimba", "xylophone", "berimbau", "cavaquinho", "charango", "cuatro",
    "bandoneon", "accordion", "concertina", "koto", "shamisen", "biwa",
    "shakuhachi", "guzheng", "pipa", "erhu", "yangqin", "gamelan", "angklung",
    "kulintang", "gayageum", "balalaika", "domra", "gusli", "cimbalom",
    "hammered dulcimer", "hurdy-gurdy", "bagpipe", "gaida", "kaval",
    "jew's harp", "musical saw", "steel guitar", "hawaiian guitar", "ukulele",
    "banjo", "mandolin", "zither", "autoharp", "harmonica", "ocarina",
    "pan pipes", "steelpan", "music box", "celesta", "glockenspiel",
    "theremin", "calliope", "barrel organ", "bells", "whistling",
]

STYLES = [
    "bhangra", "punjabi", "qawwali", "carnatic", "hindustani", "rebetiko",
    "klezmer", "tarab", "chaabi", "gnawa", "highlife", "palm wine", "juju",
    "taarab", "ethiopian", "mento", "calypso", "son cubano", "danzon",
    "cumbia", "fado", "flamenco", "hawaiian", "csardas",
    "romani", "gypsy", "tatar", "uyghur", "persian", "armenian", "turkish",
    "arabic", "egyptian", "greek", "chinese", "cantonese", "javanese",
    "balinese", "thai", "burmese", "japanese", "korean", "andean",
    "mariachi", "field recording",
]

# Voices and other free melodic lines: what the phrase finder is for.
VOCALS = [
    "carnatic vocal", "carnatic", "kriti", "bhajan", "kirtan", "keerthanam",
    "devotional", "ghazal", "thumri", "khayal", "dhrupad", "alap", "qawwali",
    "sufi", "baul", "lavani", "rajasthani folk", "punjabi folk", "boliyan",
    "tamil folk", "telugu folk", "kannada", "malayalam", "sinhala", "nepali folk",
    "mawwal", "maqam", "taqsim", "avaz", "mugham", "makam", "uzun hava",
    "byzantine chant", "znamenny", "georgian polyphony", "bulgarian voices",
    "yodel", "throat singing", "khoomei", "joik", "sean-nos", "cante jondo",
    "griot", "azmari", "tizita", "hausa", "yoruba", "isicathamiya", "mbube",
    "lullaby", "work song", "field holler", "lament", "vocal improvisation",
]

# The strange end: instruments and practices with sounds nobody expects, often
# single-voice and buzzing, nasal or metallic, like the tumbi itself (a
# one-string Punjabi lute; its cousins ektara and gopichand are in here too).
# Picked twice as often as the rest.
WEIRD_TERMS = [
    "ektara", "gopichand", "toombi", "morchang", "algoza", "pungi", "been",
    "ravanahatha", "kamaicha", "sarinda", "esraj", "dilruba", "pena",
    "khaen", "phin", "sape", "kubing", "kudyapi", "hasapi", "sasando",
    "morin khuur", "igil", "tovshuur", "jaw harp", "mouth bow", "musical bow",
    "earth bow", "slit drum", "lithophone", "waterphone", "nyckelharpa",
    "sintir", "guembri", "imzad", "krar", "begena", "masenqo", "goje",
    "molo", "kontigi", "valiha", "marovany", "inanga", "endongo", "akadinda",
    "amadinda", "ennanga", "bobre", "cuica", "marimbula", "pifano",
    "arghul", "mizmar", "suona", "sheng", "hichiriki", "sho", "piri",
    "taepyeongso", "zurla", "launeddas", "tenora", "gralla", "bombarde",
    "ondes martenot", "tape loop", "musique concrete", "early electronic",
    "zar ritual", "bori", "gnawa lila", "kecak", "jaipongan", "tarling",
    "dangdut koplo", "molam", "mor lam", "hocket", "pygmy", "aka pygmies",
    "khoomei", "overtone singing", "kargyraa", "zikr", "dhikr", "possession",
    "trance music", "ceremony", "spirit", "shaman",
]

# Everyday sounds: still searchable by name (--terms), but not dug by default,
# because they mostly turn up ordinary folk, pop and dance music.
EVERYDAY = {
    "ukulele", "banjo", "mandolin", "accordion", "concertina", "harmonica", "autoharp",
    "cuatro", "cavaquinho", "charango", "bandoneon", "celesta", "glockenspiel",
    "music box", "bells", "whistling", "xylophone", "marimba", "zither", "ocarina",
    "pan pipes", "steelpan", "bagpipe", "danzon", "son cubano", "cumbia", "mariachi",
    "calypso", "mento", "flamenco", "fado", "csardas", "hawaiian", "highlife",
    "palm wine", "juju", "lullaby", "work song", "polka", "tango", "novelty",
}

ALL_TERMS = [t for t in dict.fromkeys(INSTRUMENTS + STYLES + VOCALS + WEIRD_TERMS) if t not in EVERYDAY]

LICENSE_PD = "pd"          # public domain or CC0
LICENSE_PD_US = "pd-us"    # US public domain by age
LICENSE_BY = "by"          # CC BY: free, credit required
LICENSE_SA = "sa"          # CC BY-SA: credit, and derivatives under BY-SA
LICENSE_NC = "nc"          # CC BY-NC(-SA): non-commercial only
LICENSE_UNKNOWN = "unklar"  # nobody said: fine to play with, clear before release

LICENSE_LABELS = {
    LICENSE_PD: "Public Domain",
    LICENSE_PD_US: "Public Domain (USA, vor %d)" % (PD_CUTOFF_YEAR + 1),
    LICENSE_BY: "CC BY – Credit nötig",
    LICENSE_SA: "CC BY-SA – Credit + Share-Alike",
    LICENSE_NC: "CC BY-NC – nur nicht-kommerziell",
    LICENSE_UNKNOWN: "Lizenz unklar – zum Experimentieren",
}

TRUST_LABELS = {
    "age": "geprüft: Alter der Aufnahme",
    "commons": "geprüft: Wikimedia Commons",
    "claimed": "Angabe des Uploaders – vor Release prüfen",
    "unknown": "nicht geklärt – vor Release klären",
}


@dataclass
class Candidate:
    source: str            # "78", "commons", "ia"
    identifier: str
    title: str
    year: int | None
    downloads: int
    license: str
    license_url: str
    term: str
    trust: str
    page_url: str
    creator: str = ""
    files: list[dict] = field(default_factory=list)   # filled for Commons
    note: str = ""         # overrides the licence label when it needs saying

    @property
    def key(self) -> str:
        return "%s:%s" % (self.source, self.identifier)


def _get_json(url: str, retries: int = 4) -> dict:
    delay = 3.0
    for attempt in range(retries):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
            with urllib.request.urlopen(req, timeout=60) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as exc:
            if attempt == retries - 1 or exc.code not in (429, 500, 502, 503, 504):
                raise
            wait = exc.headers.get("Retry-After") if exc.headers else None
            time.sleep(min(60.0, float(wait)) if wait and wait.isdigit() else delay)
            delay *= 2
        except Exception:
            if attempt == retries - 1:
                raise
            time.sleep(delay)
            delay *= 2
    return {}


def _year(value) -> int | None:
    if not value:
        return None
    if isinstance(value, list):
        value = value[0]
    m = re.search(r"(1[89]\d\d|20\d\d)", str(value))
    return int(m.group(1)) if m else None


def _first(v) -> str:
    if isinstance(v, list):
        return html.unescape(", ".join(str(x) for x in v[:3]))
    return html.unescape(str(v or ""))


def classify_license(text: str | None) -> str | None:
    """Map a licence URL or short name to a class; None means do not use."""
    t = (text or "").lower().replace("_", "-")
    if not t:
        return None
    if "publicdomain" in t or "public domain" in t or "/zero/" in t or "cc0" in t or t.startswith("pd"):
        return LICENSE_PD
    if "nd" in re.findall(r"[a-z]+", t) or "-nd" in t:
        return None
    if "nc" in re.findall(r"[a-z]+", t) or "-nc" in t:
        return LICENSE_NC
    if "sa" in re.findall(r"[a-z]+", t) or "-sa" in t:
        return LICENSE_SA
    if "by" in re.findall(r"[a-z]+", t):
        return LICENSE_BY
    return None


# --- Internet Archive ---------------------------------------------------------

_EXCLUDE = ("-subject:podcast -subject:interview -subject:radio -subject:talk "
            "-subject:lecture -subject:audiobook -subject:speech -subject:sermon "
            "-subject:recitation -subject:monologue -subject:comedy "
            "-collection:etree -collection:librivoxaudio")

# Uploader-claimed "public domain" on modern commercial music is common and
# wrong. Items that look like that are dropped outright.
SUSPICIOUS = re.compile(
    r"best of|greatest|\bhits\b|top \d|full album|\bost\b|soundtrack|remix|bootleg|"
    r"unofficial|official|bollywood|\bmp3\b|\bsongs\b|discography|\bvol\.? ?\d",
    re.IGNORECASE)


# Plenty of records from the 1900s–1920s carry racist titles (minstrel and
# "coon" songs). They are skipped.
OFFENSIVE = re.compile(r"\bcoons?\b|dark(e|ie)y|pickaninn|minstrel|\bnigg|\bchink|\bjap\b|mammy|\bsambo\b|plantation",
                       re.IGNORECASE)


# Names whose recordings are known worldwide and expensive to clear. A sample
# from them is neither a secret nor cheap, so they are skipped everywhere.
FAMOUS = re.compile(
    r"lata mangeshkar|asha bhosle|mohammed rafi|kishore kumar|mukesh|a\.? ?r\.? rahman|ilaiyaraaja|"
    r"nusrat|fateh ali khan|abida parveen|ravi shankar|anoushka|bismillah|zakir hussain|ali akbar khan|"
    r"subbulakshmi|bhimsen joshi|jagjit singh|mehdi hassan|ghulam ali|hariprasad|shivkumar|"
    r"umm kulthum|oum kalthoum|fairuz|fayrouz|abdel halim|warda|cheb khaled|rachid taha|"
    r"fela|ali farka|toumani|youssou|salif keita|cesaria|miriam makeba|mulatu|"
    r"beatles|rolling stones|michael jackson|madonna|bob marley|elvis|led zeppelin|pink floyd|"
    r"daler mehndi|panjabi mc|gurdas maan|diljit|sidhu moose|shreya ghoshal|arijit|sonu nigam|"
    r"bollywood|tollywood|kollywood|t-series|zee music|saregama|sony music|universal music|vevo",
    re.IGNORECASE)


def _ia_query(q: str, rows: int, rng: random.Random, sort: str = "downloads asc") -> list[dict]:
    fields = ["identifier", "title", "date", "year", "downloads", "licenseurl",
              "collection", "creator"]
    params = [("q", q), ("rows", str(rows)), ("output", "json"), ("sort[]", sort)]
    params += [("fl[]", f) for f in fields]
    first = _get_json(IA_SEARCH + "?" + urllib.parse.urlencode(params + [("page", "1")]))
    resp = first.get("response", {})
    docs = resp.get("docs", [])
    pages = max(1, min(200, (resp.get("numFound", 0) + rows - 1) // rows))
    page = rng.randint(1, pages)
    if page > 1:
        docs = _get_json(IA_SEARCH + "?" + urllib.parse.urlencode(
            params + [("page", str(page))])).get("response", {}).get("docs", [])
    return docs


def _where(term: str) -> str:
    phrase = '"%s"' % term if " " in term else term
    return "(title:%s OR subject:%s OR description:%s)" % (phrase, phrase, phrase)


def search_78(term: str | None, rng: random.Random, rows: int = 50) -> list[Candidate]:
    """Pre-1926 records. term=None digs a random crate of the whole collection."""
    q = ("mediatype:audio AND (collection:78rpm OR collection:georgeblood OR "
         "identifier:loc-jukebox*) AND year:[1890 TO %d] %s" % (PD_CUTOFF_YEAR, _EXCLUDE))
    if term:
        q += " AND " + _where(term)
    sort = rng.choice(["downloads asc", "date asc", "identifier asc", "identifier desc"])
    out = []
    for d in _ia_query(q, rows, rng, sort):
        year = _year(d.get("year") or d.get("date"))
        if not year or year > PD_CUTOFF_YEAR:
            continue
        if OFFENSIVE.search(_first(d.get("title")) + " " + d["identifier"]):
            continue
        out.append(Candidate(
            source="78", identifier=d["identifier"], title=_first(d.get("title")) or d["identifier"],
            year=year, downloads=int(d.get("downloads") or 0), license=LICENSE_PD_US,
            license_url="", term=term or "78er-Kiste", trust="age",
            page_url="https://archive.org/details/" + d["identifier"], creator=_first(d.get("creator"))))
    return out


def search_ia_claimed(term: str, allow_nc: bool, rng: random.Random, rows: int = 50) -> list[Candidate]:
    q = ("mediatype:audio AND %s AND (licenseurl:*publicdomain* OR licenseurl:*creativecommons*) %s"
         % (_where(term), _EXCLUDE))
    out = []
    for d in _ia_query(q, rows, rng):
        lic = classify_license(d.get("licenseurl"))
        if lic is None or (lic == LICENSE_NC and not allow_nc):
            continue
        title = _first(d.get("title")) or d["identifier"]
        if SUSPICIOUS.search(title + " " + d["identifier"]) or FAMOUS.search(title + " " + _first(d.get("creator"))):
            continue
        out.append(Candidate(
            source="ia", identifier=d["identifier"], title=title,
            year=_year(d.get("year") or d.get("date")), downloads=int(d.get("downloads") or 0),
            license=lic, license_url=d.get("licenseurl") or "", term=term, trust="claimed",
            page_url="https://archive.org/details/" + d["identifier"], creator=_first(d.get("creator"))))
    return out


def search_ia_any(term: str, allow_nc: bool, rng: random.Random, rows: int = 50,
                  max_downloads: int = 3000) -> list[Candidate]:
    """Experiment mode: any Archive audio, licensed or not, as long as it is
    obscure (few downloads) and not by someone world-famous."""
    q = ("mediatype:audio AND %s AND downloads:[0 TO %d] %s"
         % (_where(term), max_downloads, _EXCLUDE))
    out = []
    for d in _ia_query(q, rows, rng):
        title = _first(d.get("title")) or d["identifier"]
        hay = " ".join([title, d["identifier"], _first(d.get("creator"))])
        if FAMOUS.search(hay) or OFFENSIVE.search(hay) or re.search(r"official|remix|vevo", hay, re.I):
            continue
        lic = classify_license(d.get("licenseurl")) if d.get("licenseurl") else LICENSE_UNKNOWN
        note = ""
        if lic is None:                      # explicitly "no derivatives"
            lic, note = LICENSE_UNKNOWN, "Keine Bearbeitung erlaubt (ND) – nur privat"
        if lic == LICENSE_NC and not allow_nc:
            lic, note = LICENSE_UNKNOWN, "Nur nicht-kommerziell (NC) – nur privat"
        out.append(Candidate(
            source="ia", identifier=d["identifier"], title=title,
            year=_year(d.get("year") or d.get("date")), downloads=int(d.get("downloads") or 0),
            license=lic, license_url=d.get("licenseurl") or "", term=term,
            trust="claimed" if lic != LICENSE_UNKNOWN else "unknown", note=note,
            page_url="https://archive.org/details/" + d["identifier"], creator=_first(d.get("creator"))))
    return out


LOSSLESS = ["Flac", "24bit Flac", "WAVE"]
LOSSY = ["VBR MP3", "Ogg Vorbis", "128Kbps MP3", "64Kbps MP3"]


def ia_audio_files(identifier: str, max_files: int = 2, max_seconds: float = 3600,
                   max_bytes: int = 120_000_000, max_hq_bytes: int = 400_000_000) -> list[dict]:
    """One entry per track: a small lossy file to analyse quickly ("url") and,
    where the item has one, the lossless original to cut from ("hq_url")."""
    meta = _get_json(IA_METADATA + urllib.parse.quote(identifier))
    tracks: dict[str, dict] = {}

    def link(name):
        return IA_DOWNLOAD + urllib.parse.quote(identifier) + "/" + urllib.parse.quote(name)

    for f in meta.get("files", []):
        fmt = f.get("format", "")
        if fmt not in LOSSLESS and fmt not in LOSSY:
            continue
        try:
            length = float(f.get("length") or 0)
        except ValueError:
            length = 0
        if length and (length < 20 or length > max_seconds):
            continue
        if int(f.get("size") or 0) > (max_hq_bytes if fmt in LOSSLESS else max_bytes):
            continue
        # Derivatives name their original; group by it.
        stem = re.sub(r"\.[^.]+$", "", f.get("original") if f.get("source") == "derivative" and f.get("original") else f["name"])
        stem = re.sub(r"(_vbr|_64kb|_128kb|_sample)$", "", stem)
        t = tracks.setdefault(stem, {})
        if fmt in LOSSY:
            rank = LOSSY.index(fmt)
            if "lossy_rank" not in t or rank < t["lossy_rank"]:
                t.update(lossy=f["name"], lossy_rank=rank)
        else:
            rank = LOSSLESS.index(fmt)
            if "hq_rank" not in t or rank < t["hq_rank"]:
                t.update(hq=f["name"], hq_rank=rank)
    out = []
    for t in tracks.values():
        first = t.get("lossy") or t.get("hq")
        if not first:
            continue
        out.append({"name": first, "url": link(first),
                    "hq_url": link(t["hq"]) if t.get("hq") and t.get("lossy") else None})
    random.shuffle(out)
    return out[:max_files]


# --- Wikimedia Commons --------------------------------------------------------

_PRONUNCIATION = re.compile(r"^File:(LL-|[A-Za-z]{2,3}(-[a-z]{2})?-[^ ]+\.(ogg|wav|mp3)$)")


def search_commons(term: str, allow_nc: bool, rng: random.Random, rows: int = 25) -> list[Candidate]:
    # Step 1: cheap full-text search for audio files.
    found = _get_json(COMMONS_API + "?" + urllib.parse.urlencode({
        "action": "query", "format": "json", "list": "search",
        "srsearch": 'filetype:audio "%s"' % term, "srnamespace": "6", "srlimit": "50",
        "sroffset": str(rng.choice([0, 0, 50]))}))
    titles = [h["title"] for h in found.get("query", {}).get("search", [])
              if not _PRONUNCIATION.match(h["title"])]
    rng.shuffle(titles)
    titles = titles[:rows]
    if not titles:
        return []
    # Step 2: licence, author, length and URL for the shortlist.
    data = _get_json(COMMONS_API + "?" + urllib.parse.urlencode({
        "action": "query", "format": "json", "titles": "|".join(titles),
        "prop": "imageinfo", "iiprop": "url|size|extmetadata",
        "iiextmetadatafilter": "LicenseShortName|LicenseUrl|Artist|DateTimeOriginal"}))
    out = []
    for page in (data.get("query", {}).get("pages", {}) or {}).values():
        title = page.get("title", "")
        info = (page.get("imageinfo") or [{}])[0]
        dur = float(info.get("duration") or 0)
        if dur and (dur < 20 or dur > 1200):
            continue
        meta = info.get("extmetadata", {})
        lic_name = meta.get("LicenseShortName", {}).get("value", "")
        lic = classify_license(lic_name) or classify_license(meta.get("LicenseUrl", {}).get("value"))
        if lic is None or (lic == LICENSE_NC and not allow_nc):
            continue
        artist = re.sub(r"<[^>]+>", "", html.unescape(meta.get("Artist", {}).get("value", ""))).strip()
        if FAMOUS.search(title + " " + artist):
            continue
        url = info.get("url", "").split("?")[0]
        if not url:
            continue
        name = title[5:] if title.startswith("File:") else title
        out.append(Candidate(
            source="commons", identifier=name, title=re.sub(r"\.[a-z0-9]+$", "", name).replace("_", " "),
            year=_year(meta.get("DateTimeOriginal", {}).get("value")), downloads=0,
            license=lic, license_url=meta.get("LicenseUrl", {}).get("value", ""),
            term=term, trust="commons", page_url=info.get("descriptionurl", ""),
            creator=artist[:80], files=[{"name": name, "url": url}]))
    return out


def files_for(c: Candidate, max_files: int = 2) -> list[dict]:
    if c.files:
        return c.files[:max_files]
    return ia_audio_files(c.identifier, max_files=max_files)


def download(url: str, dest: str) -> None:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=180) as resp, open(dest, "wb") as fh:
        while True:
            chunk = resp.read(1 << 16)
            if not chunk:
                break
            fh.write(chunk)
