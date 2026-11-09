"""SQLite schema and queries for TraceHub."""

import sqlite3
from contextlib import closing
from datetime import datetime, timezone
from uuid import uuid4


def initialize_database(database_path):
    database_path.parent.mkdir(parents=True, exist_ok=True)
    with closing(sqlite3.connect(database_path)) as connection:
        connection.execute("""
            CREATE TABLE IF NOT EXISTS receipts (
                sequence INTEGER PRIMARY KEY,
                receipt_id TEXT NOT NULL UNIQUE,
                helper_id TEXT NOT NULL,
                submission_id TEXT NOT NULL,
                reported_hostname TEXT NOT NULL,
                observed_peer_ip TEXT NOT NULL,
                message TEXT NOT NULL,
                received_at TEXT NOT NULL,
                UNIQUE (helper_id, submission_id)
            )
        """)
        connection.execute("""
            CREATE TABLE IF NOT EXISTS iot_results (
                sequence INTEGER PRIMARY KEY,
                result_id TEXT NOT NULL UNIQUE,
                device_id TEXT NOT NULL,
                request_id TEXT NOT NULL,
                observed_peer_ip TEXT NOT NULL,
                action TEXT NOT NULL,
                result TEXT NOT NULL,
                received_at TEXT NOT NULL,
                UNIQUE (device_id, request_id)
            )
        """)


def record_receipt(
    database_path, *,
    helper_id, submission_id, reported_hostname, observed_peer_ip, message,
):
    receipt_id = str(uuid4())
    received_at = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
    with closing(sqlite3.connect(database_path)) as connection:
        connection.row_factory = sqlite3.Row
        insert = connection.execute(
            """
            INSERT INTO receipts (
                receipt_id, helper_id, submission_id, reported_hostname,
                observed_peer_ip, message, received_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT (helper_id, submission_id) DO NOTHING
            """,
            (
                receipt_id, helper_id, submission_id, reported_hostname,
                observed_peer_ip, message, received_at,
            ),
        )
        receipt = connection.execute(
            """
            SELECT receipt_id, reported_hostname FROM receipts
            WHERE helper_id = ? AND submission_id = ?
            """,
            (helper_id, submission_id),
        ).fetchone()
        created = insert.rowcount == 1
        connection.commit()
    return receipt, created


def list_receipts(database_path):
    with closing(sqlite3.connect(database_path)) as connection:
        connection.row_factory = sqlite3.Row
        return connection.execute("""
            SELECT receipt_id, helper_id, reported_hostname, observed_peer_ip,
                   message, received_at
            FROM receipts ORDER BY sequence DESC LIMIT 50
        """).fetchall()


def record_iot_result(
    database_path, *,
    device_id, request_id, observed_peer_ip, action, result,
):
    result_id = str(uuid4())
    received_at = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
    with closing(sqlite3.connect(database_path)) as connection:
        connection.row_factory = sqlite3.Row
        insert = connection.execute(
            """
            INSERT INTO iot_results (
                result_id, device_id, request_id, observed_peer_ip,
                action, result, received_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT (device_id, request_id) DO NOTHING
            """,
            (
                result_id, device_id, request_id, observed_peer_ip,
                action, result, received_at,
            ),
        )
        iot_result = connection.execute(
            """
            SELECT result_id, device_id, request_id, observed_peer_ip,
                   action, result, received_at
            FROM iot_results WHERE device_id = ? AND request_id = ?
            """,
            (device_id, request_id),
        ).fetchone()
        created = insert.rowcount == 1
        connection.commit()
    return iot_result, created


def list_iot_results(database_path):
    with closing(sqlite3.connect(database_path)) as connection:
        connection.row_factory = sqlite3.Row
        return connection.execute("""
            SELECT result_id, device_id, request_id, observed_peer_ip,
                   action, result, received_at
            FROM iot_results ORDER BY sequence DESC LIMIT 50
        """).fetchall()
