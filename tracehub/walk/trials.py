"""Evaluate one local workstation trial and retain its original evidence."""

from contextlib import closing
from datetime import datetime, timedelta
import json
import math
import os
from pathlib import Path
import sqlite3
from uuid import UUID

WINDOW_MS = 90_000
MAX_HEARTBEAT_GAP_MS = 3500


def default_db():
    return Path(os.environ["LOCALAPPDATA"]) / "TraceDeck" / "tracehub.sqlite3"


def connection(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(path, timeout=8)
    db.execute("CREATE TABLE IF NOT EXISTS walk_trials "
               "(trial_id TEXT PRIMARY KEY, started TEXT NOT NULL, evidence TEXT NOT NULL)")
    return db


def save(trial, path=None):
    if UUID(trial["id"]).hex != trial["id"] or trial["kind"] not in ("walk", "control"):
        raise ValueError("Invalid trial identifier or kind")
    evaluate(trial)
    payload = json.dumps(trial, sort_keys=True, separators=(",", ":"), allow_nan=False)
    with closing(connection(path or default_db())) as db, db:
        db.execute("INSERT INTO walk_trials VALUES(?,?,?) ON CONFLICT(trial_id) DO NOTHING",
                   (trial["id"], trial["started"], payload))
        stored, = db.execute("SELECT evidence FROM walk_trials WHERE trial_id=?", (trial["id"],)).fetchone()
        if stored != payload:
            raise ValueError("This trial ID already has different evidence")


def recent(path):
    with closing(connection(path)) as db:
        return [json.loads(row[0]) for row in db.execute(
            "SELECT evidence FROM walk_trials ORDER BY started DESC LIMIT 50")]


def tick_delta(later, earlier):
    return ((later - earlier + 2**31) % 2**32) - 2**31


def evaluate(trial):
    events = trial["events"]
    result = {"status": "Inconclusive", "reason": "", "marker_utc": None, "lock_utc": None,
              "elapsed_s": None, "uncertainty_ms": None, "radio": None}

    def finish(status, reason):
        result.update(status=status, reason=reason)
        return result

    if not events or events[0][1] != "initial-unlock":
        return finish("Inconclusive", "The observer did not establish an initially unlocked session.")
    locks = [e for e in events if e[1] == "lock"]
    if trial["kind"] == "control":
        if locks and any(e[1] == "unlock" and e[0] >= locks[0][0] for e in events):
            result["lock_utc"] = locks[0][2]
            return finish("Manual control observed", "Windows recorded lock and unlock in the observed session.")
        return finish("Inconclusive", "The manual control requires both Windows notifications.")
    if trial.get("error"):
        return finish("Inconclusive", trial["error"])
    if not all(trial.get(k) for k in ("before", "after", "marker")):
        return finish("Inconclusive", "Missing doorway mark or clock exchange.")
    pre, post = trial["before"], trial["after"]  # board tick, PC midpoint, half RTT
    tick, age, rssi = trial["marker"]
    span = tick_delta(post[0], pre[0])
    offset = tick_delta(tick, pre[0])
    host_span = post[1] - pre[1]
    if span < 0 or not 0 <= offset <= span or host_span <= 0:
        return finish("Inconclusive", "Doorway tick falls outside this trial's clock samples.")
    drift = abs(host_span - span)
    error = math.ceil(max(pre[2], post[2]) + drift + 1)
    if error > 1000:
        return finish("Inconclusive", "Clock agreement exceeded the one-second timing budget.")
    marker = pre[1] + offset
    event_zero = datetime.fromisoformat(events[0][2])
    result.update(marker_utc=(event_zero + timedelta(milliseconds=marker-events[0][0])).isoformat(timespec="milliseconds"),
                  uncertainty_ms=error, radio={"age_ms": age, "rssi": None if rssi == 127 else rssi})
    transitions = [e for e in events if e[1] in ("lock", "unlock")]
    previous = [e for e in transitions if e[0] < marker - error]
    already = bool(previous) and previous[-1][1] == "lock"
    following = next((e for e in locks if e[0] >= marker - error), None)
    deadline = marker + WINDOW_MS
    needed = deadline + error
    if already:
        needed = marker + error
    elif following:
        needed = min(following[0], needed)
    previous_time = events[0][0]
    covered = False
    for stamp, kind, _ in events:
        if stamp < previous_time or stamp - previous_time > MAX_HEARTBEAT_GAP_MS:
            break
        if kind in ("power-change", "session-change", "stop") and stamp <= needed:
            break
        previous_time = stamp
        if stamp >= needed:
            covered = True
            break
    if not covered:
        return finish("Inconclusive", "Observer coverage ended or was interrupted before the required event or deadline.")
    if any(abs(e[0] - marker) <= error for e in transitions):
        return finish("Inconclusive", "Doorway and session transition overlap within clock uncertainty.")
    if already:
        result["lock_utc"] = previous[-1][2]
        return finish("Already locked at marker", "The latest Windows transition before the doorway was a lock.")
    if following and following[0] <= deadline + error:
        result.update(lock_utc=following[2], elapsed_s=round((following[0] - marker) / 1000, 3))
        return finish("Lock observed", "Windows reported a lock after the doorway press.")
    if following:
        result["lock_utc"] = following[2]
    return finish("No lock observed within 90 s", "The observer covered the full window after the doorway press.")
