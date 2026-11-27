"""Tiny background-job runner so long searches don't block the UI.

    job_id = jobs.start("Restaurants suchen", my_async_fn, arg1, ...)
    # inside my_async_fn(job, ...): job.update(done=3, total=10, message="...")
"""
from __future__ import annotations

import asyncio
import logging
import time
import uuid

log = logging.getLogger("beat-dashboard.jobs")
_JOBS: dict[str, "Job"] = {}


class Job:
    def __init__(self, title: str):
        self.id = uuid.uuid4().hex[:10]
        self.title = title
        self.status = "running"
        self.done = 0
        self.total = 0
        self.message = ""
        self.result = None
        self.error = None
        self.started = time.time()
        self.cancelled = False

    def update(self, done: int | None = None, total: int | None = None, message: str | None = None):
        if done is not None:
            self.done = done
        if total is not None:
            self.total = total
        if message is not None:
            self.message = message

    def public(self) -> dict:
        return {"id": self.id, "title": self.title, "status": self.status, "done": self.done, "total": self.total,
                "message": self.message, "result": self.result, "error": self.error,
                "seconds": round(time.time() - self.started)}


def start(title: str, fn, *args, **kwargs) -> str:
    job = Job(title)
    _JOBS[job.id] = job

    async def runner():
        try:
            job.result = await fn(job, *args, **kwargs)
            job.status = "cancelled" if job.cancelled else "done"
        except Exception as e:  # report, don't crash the server
            log.exception("Job %s failed", title)
            job.status, job.error = "error", f"{e.__class__.__name__}: {e}"

    asyncio.get_running_loop().create_task(runner())
    # keep the list short
    for old in sorted(_JOBS.values(), key=lambda j: j.started)[:-30]:
        _JOBS.pop(old.id, None)
    return job.id


def get(job_id: str) -> Job | None:
    return _JOBS.get(job_id)


def running() -> list[dict]:
    return [j.public() for j in _JOBS.values() if j.status == "running"]
