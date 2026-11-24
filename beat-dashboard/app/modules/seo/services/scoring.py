"""Keyword opportunity score: demand vs competition (0-100)."""
from __future__ import annotations

import math

from .knowledge import HEAD_TERMS


def _log01(x: float, decades: float) -> float:
    return max(0.0, min(1.0, math.log10(max(0.0, x) + 1) / decades))


def competition_from_youtube(n: dict) -> float:
    """0 = wide open, 1 = hopeless for a small channel."""
    up = n["uploads_30d"]
    supply = 0.15 if up < 50 else 0.45 if up < 300 else 0.7 if up < 1000 else 0.92
    if n.get("uploads_30d_capped"):
        supply = max(supply, 0.6)
    big = _log01(n["median_subs_top10"], 6)            # 1M subs -> 1.0
    exact = n["exact_title_share"]
    c = 0.45 * supply + 0.35 * big + 0.2 * exact
    c -= 0.06 * min(3, n["small_channels_top10"])        # small channels rank -> winnable
    return max(0.0, min(1.0, c))


def estimate_competition(artist: str | None, popularity: float, suggest_score: float) -> float:
    """No YouTube key: rough guess from how big the artist is."""
    if artist and artist.lower() in HEAD_TERMS:
        return 0.9
    return max(0.2, min(0.9, 0.25 + 0.55 * popularity + 0.15 * suggest_score))


def score(*, suggest_score: float, popularity: float, niche: dict | None, artist: str | None,
          rising: bool = False) -> dict:
    if niche:
        volume = _log01(niche["median_views"], 6)
        demand = 0.5 * suggest_score + 0.2 * popularity + 0.3 * volume
        comp = competition_from_youtube(niche)
        fresh = niche["fresh_share"]
        outliers = len(niche["outliers"])
        estimated = False
    else:
        demand = 0.7 * suggest_score + 0.3 * popularity
        comp = estimate_competition(artist, popularity, suggest_score)
        fresh, outliers, estimated = 0.5, 0, True
    if rising:
        demand = min(1.0, demand + 0.1)
    opp = 100 * (demand ** 0.6) * ((1 - comp) ** 0.4) * (0.85 + 0.15 * fresh)
    opp += min(10, 2 * outliers)
    opp = max(0.0, min(100.0, opp))
    label = "Gold" if opp >= 65 else "Gut" if opp >= 45 else "Gesättigt" if comp >= 0.6 else "Wenig Nachfrage"
    return {"opportunity": round(opp), "demand": round(demand, 2), "competition": round(comp, 2),
            "freshness": round(fresh, 2), "outliers": outliers, "label": label, "estimated": estimated}


def stage(subs: int) -> dict:
    if subs < 500:
        return {"name": "Start (<500 Abos)", "advice": "Nur Subgenre-Crossovers und aufsteigende Künstler. Große Namen (Drake, Travis, Future) solo meiden."}
    if subs < 5000:
        return {"name": "Aufbau (500–5k)", "advice": "Mittelgroße Künstler und 'x'-Kombos. 3+ Uploads pro Woche."}
    if subs < 20000:
        return {"name": "Wachstum (5k–20k)", "advice": "Outlier-Scanning + Format-Modifier (Sped Up, Jersey Club, Guitar)."}
    return {"name": "Etabliert (20k+)", "advice": "Auch größere Künstler-Keywords sind jetzt realistisch."}
