import sqlite3
from contextlib import closing
from datetime import datetime, timezone
from uuid import uuid4

# SQLite schema and queries for TraceHub
def initialize_database(database_path):
    database_path.parent.mkdir(parents=True, exist_ok=True)
    with closing(sqlite3.connect(database_path)) as connection:
        connection.execute("BEGIN IMMEDIATE")
        legacy_loot_table = connection.execute(
            "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'receipts'"
        ).fetchone()
        if legacy_loot_table:
            connection.execute("ALTER TABLE receipts RENAME TO loot")
            connection.execute("ALTER TABLE loot RENAME COLUMN receipt_id TO loot_id")

        connection.execute("""
            CREATE TABLE IF NOT EXISTS loot (
                sequence INTEGER PRIMARY KEY,
                loot_id TEXT NOT NULL UNIQUE,
                helper_id TEXT NOT NULL,
                submission_id TEXT NOT NULL,
                reported_hostname TEXT NOT NULL,
                observed_peer_ip TEXT NOT NULL,
                message TEXT NOT NULL,
                filename TEXT,
                file_size_bytes INTEGER,
                received_at TEXT NOT NULL,
                UNIQUE (helper_id, submission_id)
            )
        """)
        # Add file metadata to existing loot records.
        loot_columns = {
            column[1] for column in connection.execute("PRAGMA table_info(loot)")
        }
        if "filename" not in loot_columns:
            connection.execute("ALTER TABLE loot ADD COLUMN filename TEXT")
        if "file_size_bytes" not in loot_columns:
            connection.execute("ALTER TABLE loot ADD COLUMN file_size_bytes INTEGER")

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
        connection.commit()


def record_loot(
    database_path, *,
    helper_id, submission_id, reported_hostname, observed_peer_ip, file_text,
    filename, file_size_bytes,
):
    loot_id = str(uuid4())
    received_at = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
    with closing(sqlite3.connect(database_path)) as connection:
        connection.row_factory = sqlite3.Row
        insert_result = connection.execute(
            """
            INSERT INTO loot (
                loot_id, helper_id, submission_id, reported_hostname,
                observed_peer_ip, message, filename, file_size_bytes, received_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT (helper_id, submission_id) DO NOTHING
            """,
            (
                loot_id, helper_id, submission_id, reported_hostname,
                observed_peer_ip, file_text, filename, file_size_bytes, received_at,
            ),
        )
        loot_entry = connection.execute(
            """
            SELECT loot_id, reported_hostname, message AS file_text, filename, file_size_bytes
            FROM loot
            WHERE helper_id = ? AND submission_id = ?
            """,
            (helper_id, submission_id),
        ).fetchone()
        loot_created = insert_result.rowcount == 1
        connection.commit()
    return loot_entry, loot_created


def list_loot(database_path):
    with closing(sqlite3.connect(database_path)) as connection:
        connection.row_factory = sqlite3.Row
        return connection.execute("""
            SELECT loot_id, helper_id, reported_hostname, observed_peer_ip,
                   message AS file_text, filename, file_size_bytes, received_at
            FROM loot ORDER BY sequence DESC LIMIT 50
        """).fetchall()


# def record_iot_result(
#     database_path, *,
#     device_id, request_id, observed_peer_ip, action, result,
# ):
#     result_id = str(uuid4())
#     received_at = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
#     with closing(sqlite3.connect(database_path)) as connection:
#         connection.row_factory = sqlite3.Row
#         insert_result = connection.execute(
#             """
#             INSERT INTO iot_results (
#                 result_id, device_id, request_id, observed_peer_ip,
#                 action, result, received_at
#             ) VALUES (?, ?, ?, ?, ?, ?, ?)
#             ON CONFLICT (device_id, request_id) DO NOTHING
#             """,
#             (
#                 result_id, device_id, request_id, observed_peer_ip,
#                 action, result, received_at,
#             ),
#         )
#         iot_result = connection.execute(
#             """
#             SELECT result_id, device_id, request_id, observed_peer_ip,
#                    action, result, received_at
#             FROM iot_results WHERE device_id = ? AND request_id = ?
#             """,
#             (device_id, request_id),
#         ).fetchone()
#         result_created = insert_result.rowcount == 1
#         connection.commit()
#     return iot_result, result_created


# def list_iot_results(database_path):
#     with closing(sqlite3.connect(database_path)) as connection:
#         connection.row_factory = sqlite3.Row
#         return connection.execute("""
#             SELECT result_id, device_id, request_id, observed_peer_ip,
#                    action, result, received_at
#             FROM iot_results ORDER BY sequence DESC LIMIT 50
#         """).fetchall()
