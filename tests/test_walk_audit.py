import copy
from argparse import Namespace
from contextlib import nullcontext, redirect_stdout
from datetime import datetime, timedelta, timezone
from pathlib import Path
from io import StringIO
from tempfile import TemporaryDirectory
from unittest import TestCase, main
from unittest.mock import Mock, patch
from uuid import uuid4

from tracehub import create_app
from tracehub.walk.trials import evaluate, recent, save


def fixture(lock=None):
    epoch = datetime(2026, 1, 1, tzinfo=timezone.utc)
    points = [(0, "initial-unlock")] + [(n, "heartbeat") for n in range(1000, 100001, 1000)]
    if lock is not None:
        points.append((lock, "lock"))
    events = [[n, k, (epoch + timedelta(milliseconds=n)).isoformat()] for n, k in sorted(points)]
    return {"id": uuid4().hex, "kind": "walk", "session": 1, "started": epoch.isoformat(), "events": events,
            "before": [0xffffff00, 1000, 2], "after": [0xffffff00 + 98000 - 2**32, 99000, 1],
            "marker": [(0xffffff00 + 5000) % 2**32, 341, -72], "complete": True}


class WalkTests(TestCase):
    def test_tick_wrap_and_independent_lock(self):
        result = evaluate(fixture(7100))
        self.assertEqual((result["status"], result["elapsed_s"]), ("Lock observed", 1.1))
        self.assertEqual(result["radio"], {"age_ms": 341, "rssi": -72})

    def test_already_locked_and_ordering_uncertainty(self):
        self.assertEqual(evaluate(fixture(4500))["status"], "Already locked at marker")
        self.assertEqual(evaluate(fixture(6001))["status"], "Inconclusive")

    def test_healthy_timeout_and_late_lock(self):
        t = fixture(99000)
        t["marker"][1] = 25000
        result = evaluate(t)
        self.assertEqual(result["status"], "No lock observed within 90 s")
        self.assertIsNotNone(result["lock_utc"])

    def test_gap_reset_sleep_and_missing_marker(self):
        t = fixture()
        t["events"] = [e for e in t["events"] if not 10000 <= e[0] <= 18000]
        self.assertEqual(evaluate(t)["status"], "Inconclusive")
        for key, value in (("after", [50, 99000, 1]), ("marker", None)):
            t = fixture()
            t[key] = value
            self.assertEqual(evaluate(t)["status"], "Inconclusive")
        t = fixture()
        t["events"][4][1] = "power-change"
        self.assertEqual(evaluate(t)["status"], "Inconclusive")

    def test_coverage_finishes_at_observed_lock(self):
        t = fixture(8000)
        t["events"] = [e for e in t["events"] if e[0] <= 8000]
        self.assertEqual(evaluate(t)["status"], "Lock observed")

    def test_wrong_trial_or_delayed_serial_reply_is_ignored(self):
        from tracehub.walk.capture import exchange
        trial = uuid4().hex
        port = Mock()
        port.readline.side_effect = [
            f"WA1 {uuid4().hex} 1234abcd MARKED 500 410 22 -77\n".encode(),
            f"WA1 {trial} deadbeef ARMED 600 0 0 127\n".encode(),
            f"WA1 {trial} 1234abcd MARKED 700 660 12 -55\n".encode(),
        ]
        with patch("tracehub.walk.capture.secrets.token_hex", return_value="1234abcd"):
            state, clock, marker = exchange(port, trial, "READ")
        self.assertEqual((state, clock[0], marker), ("MARKED", 700, [660, 12, -55]))

    def test_real_time_order_cannot_be_replaced_with_import_time(self):
        t = fixture(8100)
        t["started"] = "2027-01-01T10:00:00+00:00"
        self.assertEqual(evaluate(t)["elapsed_s"], 2.1)
        t["after"][1] += 2500
        self.assertIn("Clock agreement", evaluate(t)["reason"])

    def test_persistence_idempotence_and_local_view(self):
        t = fixture(7100)
        with TemporaryDirectory() as tmp, patch.dict("os.environ", LOCALAPPDATA=tmp):
            app = create_app("walk")
            save(t)
            save(t)
            with self.assertRaises(ValueError):
                changed = copy.deepcopy(t)
                changed["marker"][2] = -99
                save(changed)
            self.assertEqual(len(recent(Path(app.config["DATABASE_PATH"]))), 1)
            client = create_app("walk").test_client()
            response = client.get("/walk-away")
            self.assertEqual(response.status_code, 200)
            self.assertIn(b"Lock observed", response.data)
            self.assertIn(t["id"].encode(), response.data)
            self.assertEqual(client.get("/walk-away", environ_base={"REMOTE_ADDR": "192.0.2.1"}).status_code, 403)

    def test_capture_saves_before_acknowledging_board(self):
        from tracehub.walk import capture
        t = fixture(7100)
        observer = Mock(session=1, error=None)
        observer.snapshot.return_value = t["events"]
        args = Namespace(command="walk", route="Fixture", configuration="Synthetic", timeout=120, port="fixture")
        with TemporaryDirectory() as tmp, patch.dict("os.environ", LOCALAPPDATA=tmp):
            def reply(port, trial_id, verb):
                if verb == "ARM":
                    return "ARMED", t["before"], []
                if verb == "READ":
                    return "MARKED", t["after"], t["marker"]
                rows = recent(capture.default_db())
                self.assertEqual((len(rows), rows[0]["id"], rows[0]["complete"]), (1, trial_id, True))
                return "SAVED", t["after"], []
            with (patch.object(capture, "SessionObserver", return_value=nullcontext(observer)),
                  patch.object(capture.serial, "Serial", return_value=nullcontext(None)),
                  patch.object(capture, "sync", side_effect=[t["before"], t["after"]]),
                  patch.object(capture, "exchange", side_effect=reply) as calls,
                  patch.object(capture.time, "sleep"), redirect_stdout(StringIO())):
                capture.run(args)
            self.assertEqual([call.args[2] for call in calls.call_args_list], ["ARM", "READ", "ACK"])


if __name__ == "__main__":
    main()
