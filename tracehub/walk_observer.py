"""Read lock notifications for the local Windows console session."""

import ctypes
from ctypes import wintypes
from datetime import datetime, timezone
import os
import threading
import time
from uuid import uuid4

import win32gui
import win32ts


def clock_ms():
    return time.perf_counter_ns() // 1_000_000


def initial_state(session):
    # WTSINFOEXW has a level followed by a union aligned for LARGE_INTEGER.
    # Only the documented prefix of WTSINFOEX_LEVEL1_W is needed.
    class Prefix(ctypes.Structure):
        _fields_ = [("session", wintypes.DWORD), ("state", ctypes.c_int),
                    ("flags", wintypes.LONG)]

    class Data(ctypes.Union):
        _fields_ = [("one", Prefix), ("alignment", ctypes.c_longlong)]

    class Info(ctypes.Structure):
        _fields_ = [("level", wintypes.DWORD), ("data", Data)]

    api = ctypes.WinDLL("wtsapi32", use_last_error=True)
    buf, size = ctypes.c_void_p(), wintypes.DWORD()
    if not api.WTSQuerySessionInformationW(None, session, 25, ctypes.byref(buf), ctypes.byref(size)):
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        if size.value < ctypes.sizeof(Info):
            raise RuntimeError("Incomplete WTS session data")
        info = ctypes.cast(buf, ctypes.POINTER(Info)).contents
        if info.level != 1 or info.data.one.session != session:
            raise RuntimeError("Windows returned a different session")
        return {0: "lock", 1: "unlock"}.get(info.data.one.flags, "unknown")
    finally:
        api.WTSFreeMemory(buf)


class SessionObserver:
    def __init__(self):
        self.ready = threading.Event()
        self.stop = threading.Event()
        self.mutex = threading.Lock()
        self.events = []
        self.error = None
        self.thread = threading.Thread(target=self._listen, daemon=True)
        self.session = win32ts.ProcessIdToSessionId(os.getpid())

    def record(self, kind):
        with self.mutex:
            self.events.append([clock_ms(), kind, datetime.now(timezone.utc).isoformat(timespec="milliseconds")])

    def snapshot(self):
        with self.mutex:
            return self.events.copy()

    def __enter__(self):
        self.thread.start()
        if not self.ready.wait(8) or self.error:
            self.stop.set()
            raise RuntimeError(self.error or "Windows observer startup timed out")
        return self

    def __exit__(self, *args):
        self.stop.set()
        self.thread.join(5)

    def _listen(self):
        def dispatch(hwnd, message, reason, detail):
            if message == 0x02B1 and detail == self.session:  # WM_WTSSESSION_CHANGE
                kind = {7: "lock", 8: "unlock"}.get(reason, "session-change")
                self.record(kind)
            elif message == 0x0218 and reason in (4, 7, 18):  # suspend/resume
                self.record("power-change")
            return win32gui.DefWindowProc(hwnd, message, reason, detail)

        hwnd = None
        registered = False
        window = win32gui.WNDCLASS()
        window.lpszClassName = "TraceDeckWalk" + uuid4().hex
        window.lpfnWndProc = dispatch
        try:
            if self.session != win32ts.WTSGetActiveConsoleSessionId():
                raise RuntimeError("Start the observer in the physical Windows console session")
            atom = win32gui.RegisterClass(window)
            hwnd = win32gui.CreateWindow(atom, "", 0, 0, 0, 0, 0, 0, 0, 0, None)
            win32ts.WTSRegisterSessionNotification(hwnd, 0)
            registered = True
            self.record("initial-" + initial_state(self.session))
            self.ready.set()
            next_beat = 0
            while not self.stop.wait(0.05):
                win32gui.PumpWaitingMessages()
                if clock_ms() >= next_beat:
                    self.record("heartbeat")
                    next_beat = clock_ms() + 1000
        except Exception as exc:
            self.error = str(exc)
        finally:
            self.record("stop")
            self.ready.set()
            if registered:
                win32ts.WTSUnRegisterSessionNotification(hwnd)
            if hwnd:
                win32gui.DestroyWindow(hwnd)
                win32gui.UnregisterClass(window.lpszClassName, 0)
