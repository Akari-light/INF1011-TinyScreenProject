"""Capture a local Walk-Away Lock Audit using the handheld and Windows WTS."""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import secrets
import time
from uuid import uuid4

import serial

from .observer import SessionObserver, clock_ms
from .trials import default_db, evaluate, save


def exchange(port, trial_id, verb):
    nonce = secrets.token_hex(4)
    expected = ["WA1", trial_id, nonce]
    start = time.perf_counter_ns()
    port.write(f"WA1 {verb} {trial_id} {nonce}\n".encode("ascii"))
    while time.perf_counter_ns() - start < 1_500_000_000:
        parts = port.readline().decode("ascii", errors="replace").split()
        end = time.perf_counter_ns()
        if len(parts) == 8 and parts[:3] == expected:
            tick, marker, age, rssi = map(int, parts[4:])
            return parts[3], [tick, (start + end) / 2_000_000, (end - start) / 2_000_000], [marker, age, rssi]
    raise TimeoutError("No matching WA1 reply. Open Walk-away on the handheld.")


def sync(port, trial_id):
    samples = []
    for _ in range(5):
        state, sample, _ = exchange(port, trial_id, "PING")
        if state != "CLOCK":
            raise ValueError("Unsupported handheld clock reply")
        samples.append(sample)
    return min(samples, key=lambda sample: sample[2])


def journal(path, trial, observer):
    trial["events"] = observer.snapshot()
    tmp = path.with_suffix(".tmp")
    tmp.write_text(json.dumps(trial, indent=2), encoding="utf-8")
    os.replace(tmp, path)


def run(args):
    trial = {"id": uuid4().hex, "kind": args.command, "started": datetime.now(timezone.utc).isoformat(),
             "route": args.route, "configuration": args.configuration, "events": []}
    directory = default_db().parent / "walk-trials"
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / (trial["id"] + ".json")
    deadline = clock_ms() + args.timeout * 1000
    armed = False
    message = ""
    print(f'Trial {trial["id"]} | {path}', flush=True)
    with SessionObserver() as observer:
        trial["session"] = observer.session
        try:
            while clock_ms() < deadline:
                journal(path, trial, observer)
                if observer.error:
                    raise RuntimeError(observer.error)
                if trial["events"][0][1] != "initial-unlock":
                    raise RuntimeError("Start this trial in an unlocked console session")
                if args.command == "control":
                    status = "Press Win+L, then sign back in."
                    if evaluate(trial)["status"] == "Manual control observed":
                        break
                elif "marker" in trial:
                    outcome = evaluate(trial)
                    status = outcome["reason"]
                    # Allow the healthy observer to cover the full window if docking early.
                    waiting = outcome["reason"].startswith("Observer coverage")
                    if not waiting or clock_ms() >= trial["after"][1] + 92_000:
                        break
                else:
                    try:
                        with serial.Serial(args.port, 115200, timeout=0.15, write_timeout=1) as port:
                            if not armed:
                                pre = sync(port, trial["id"])
                                state, _, _ = exchange(port, trial["id"], "ARM")
                                status = state
                                if state == "ARMED":
                                    trial["before"] = pre
                                    armed = True
                                    status = "Armed. Carry the paired phone and handheld. Press L1 at the doorway."
                                elif state == "BUSY":
                                    raise ValueError("A different trial is on the board. Import or discard it on the OLED.")
                            else:
                                state, _, marker = exchange(port, trial["id"], "READ")
                                status = "Waiting for the doorway press and redock."
                                if state == "MARKED":
                                    post = sync(port, trial["id"])
                                    trial.update(after=post, marker=marker)
                                    status = "Doorway mark recovered."
                                elif state != "ARMED":
                                    raise ValueError("Trial missing or mismatched after docking. Check for reset or cancellation.")
                    except (serial.SerialException, TimeoutError) as exc:
                        status = "Waiting for handheld to redock." if armed else str(exc)
                if status != message:
                    print(status, flush=True)
                    message = status
                time.sleep(0.7)
            else:
                if "marker" not in trial:
                    trial["error"] = "Trial timed out before all required observations were collected."
        except (KeyboardInterrupt, RuntimeError, ValueError) as exc:
            trial["error"] = str(exc) or "Operator stopped the trial."
    trial["complete"] = True
    journal(path, trial, observer)
    save(trial)
    print(evaluate(trial), flush=True)
    if args.command == "walk" and "marker" in trial:
        try:
            with serial.Serial(args.port, 115200, timeout=0.15, write_timeout=1) as port:
                state, _, _ = exchange(port, trial["id"], "ACK")
                print("Handheld acknowledgement:", state)
        except (serial.SerialException, TimeoutError):
            print("Trial saved on PC. Use L2 on the handheld to clear the retained copy.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("control", "walk"):
        cmd = commands.add_parser(name)
        if name == "walk":
            cmd.add_argument("--port", required=True, help="Identified handheld application COM port")
        cmd.add_argument("--route", default="")
        cmd.add_argument("--configuration", default="Unrecorded", help="Phone pairing, Dynamic Lock and sleep settings")
        cmd.add_argument("--timeout", type=int, default=600, help="Maximum collection time in seconds")
    load = commands.add_parser("import", help="Re-import a locally saved trial")
    load.add_argument("file", type=Path)
    args = parser.parse_args()
    if args.command == "import":
        trial = json.loads(args.file.read_text(encoding="utf-8"))
        if not trial.get("complete"):
            parser.error("This is a live or interrupted journal. Import a finalized trial.")
        save(trial)
        print(evaluate(trial))
    else:
        run(args)
