"""Sunrise / sunset without internet (NOAA approximation, ±2 min) - for golden hour planning."""
from __future__ import annotations

import math
from datetime import date, datetime, timedelta, timezone
from zoneinfo import ZoneInfo

try:
    VIENNA = ZoneInfo("Europe/Vienna")
except Exception:
    VIENNA = timezone(timedelta(hours=1))


def _utc_event(d: date, lat: float, lon: float, rising: bool) -> datetime | None:
    n = d.timetuple().tm_yday
    lng_hour = lon / 15
    t = n + ((6 if rising else 18) - lng_hour) / 24
    m = 0.9856 * t - 3.289
    l = (m + 1.916 * math.sin(math.radians(m)) + 0.020 * math.sin(math.radians(2 * m)) + 282.634) % 360
    ra = math.degrees(math.atan(0.91764 * math.tan(math.radians(l)))) % 360
    ra = (ra + (math.floor(l / 90) * 90 - math.floor(ra / 90) * 90)) / 15
    sin_dec = 0.39782 * math.sin(math.radians(l))
    cos_dec = math.cos(math.asin(sin_dec))
    cos_h = (math.cos(math.radians(90.833)) - sin_dec * math.sin(math.radians(lat))) / (cos_dec * math.cos(math.radians(lat)))
    if not -1 <= cos_h <= 1:
        return None
    h = (360 - math.degrees(math.acos(cos_h))) if rising else math.degrees(math.acos(cos_h))
    ut = (h / 15 + ra - 0.06571 * t - 6.622 - lng_hour) % 24
    return datetime(d.year, d.month, d.day, tzinfo=timezone.utc) + timedelta(hours=ut)


def sun_times(d: date, lat: float = 48.2082, lon: float = 16.3738) -> dict:
    rise, set_ = _utc_event(d, lat, lon, True), _utc_event(d, lat, lon, False)
    if not rise or not set_:
        return {}
    rise, set_ = rise.astimezone(VIENNA), set_.astimezone(VIENNA)
    fmt = lambda x: x.strftime("%H:%M")
    return {"sunrise": fmt(rise), "sunset": fmt(set_),
            "golden_morning": f"{fmt(rise)}–{fmt(rise + timedelta(minutes=60))}",
            "golden_evening": f"{fmt(set_ - timedelta(minutes=60))}–{fmt(set_)}",
            "blue_hour": f"{fmt(set_)}–{fmt(set_ + timedelta(minutes=30))}"}
