import json
import re
from secrets import compare_digest
from uuid import UUID

from flask import Blueprint, current_app, request

from . import database

LAB_MESSAGE = "TraceDeck synthetic lab message"
intake = Blueprint("intake", __name__)


def load_helper_tokens(instance_directory):
    credentials_file = instance_directory / "helpers.json"
    try:
        helper_tokens = json.loads(credentials_file.read_text(encoding="utf-8-sig"))
    except FileNotFoundError as error:
        raise ValueError(
            f"Create {credentials_file} before starting intake."
        ) from error

    if not isinstance(helper_tokens, dict) or not helper_tokens:
        raise ValueError("helpers.json must map helper IDs to tokens.")
    for helper_id, token in helper_tokens.items():
        if re.fullmatch(r"[A-Za-z0-9_.-]{1,64}", helper_id) is None:
            raise ValueError(
                "Use letters, digits, dots, hyphens or underscores in helper IDs."
            )
        if not isinstance(token, str) or re.fullmatch(r"[A-Za-z0-9_-]{43}", token) is None:
            raise ValueError("Generate each helper token with secrets.token_urlsafe(32).")
    if len(set(helper_tokens.values())) != len(helper_tokens):
        raise ValueError("Each helper needs its own token.")
    return helper_tokens


def authenticated_helper():
    authorization = request.authorization
    if authorization is None or authorization.type != "bearer":
        return None
    if not authorization.token:
        return None

    supplied_token = authorization.token.encode("utf-8")
    for helper_id, token in current_app.config["HELPER_TOKENS"].items():
        if compare_digest(supplied_token, token.encode("ascii")):
            return helper_id
    return None


@intake.post("/api/v1/receipts")
def receive_receipt():
    if not request.is_secure:
        return {"error": "https_required"}, 403
    helper_id = authenticated_helper()
    if helper_id is None:
        return {"error": "invalid_helper_credential"}, 401, {"WWW-Authenticate": "Bearer"}

    payload = request.get_json(silent=True)
    expected_fields = {"submission_id", "hostname", "message"}
    if not isinstance(payload, dict) or set(payload) != expected_fields:
        return {"error": "invalid_payload"}, 400
    if payload["message"] != LAB_MESSAGE:
        return {"error": "invalid_message"}, 400

    hostname = payload["hostname"]
    if (
        not isinstance(hostname, str)
        or not 1 <= len(hostname) <= 253
        or not hostname.isprintable()
        or hostname != hostname.strip()
    ):
        return {"error": "invalid_hostname"}, 400

    submission_id = payload["submission_id"]
    if not isinstance(submission_id, str) or len(submission_id) != 36:
        return {"error": "invalid_submission_id"}, 400
    try:
        submission_id = str(UUID(submission_id))
    except ValueError:
        return {"error": "invalid_submission_id"}, 400

    receipt, created = database.record_receipt(
        current_app.config["DATABASE_PATH"],
        helper_id=helper_id,
        submission_id=submission_id,
        reported_hostname=hostname,
        observed_peer_ip=request.remote_addr,
        message=payload["message"],
    )
    if receipt["reported_hostname"] != hostname:
        return {"error": "submission_id_conflict"}, 409
    return {"receipt_id": receipt["receipt_id"]}, 201 if created else 200
