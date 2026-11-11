import json
import re
from secrets import compare_digest
from uuid import UUID

from flask import Blueprint, current_app, request

from . import database

MAX_TEXT_FILE_BYTES = 1024
loot_receiver_blueprint = Blueprint("loot_receiver", __name__)


def load_helper_tokens(instance_directory):
    credentials_file = instance_directory / "helpers.json"
    try:
        helper_tokens = json.loads(credentials_file.read_text(encoding="utf-8-sig"))
    except FileNotFoundError as error:
        raise ValueError(
            f"Create {credentials_file} before starting the loot receiver."
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


def get_authenticated_helper_id():
    authorization = request.authorization
    if authorization is None or authorization.type != "bearer":
        return None
    if not authorization.token:
        return None

    submitted_token = authorization.token.encode("utf-8")
    for helper_id, registered_token in current_app.config["HELPER_TOKENS"].items():
        if compare_digest(submitted_token, registered_token.encode("ascii")):
            return helper_id
    return None


@loot_receiver_blueprint.post("/api/v1/loot")
def receive_loot():
    if not request.is_secure:
        return {"error": "https_required"}, 403
    helper_id = get_authenticated_helper_id()
    if helper_id is None:
        return {"error": "invalid_helper_credential"}, 401, {"WWW-Authenticate": "Bearer"}

    if request.mimetype != "multipart/form-data":
        return {"error": "multipart_required"}, 415
    form_fields = request.form
    uploaded_files = request.files.getlist("file")
    if (
        set(form_fields) != {"submission_id", "hostname"}
        or len(form_fields.getlist("submission_id")) != 1
        or len(form_fields.getlist("hostname")) != 1
        or set(request.files) != {"file"}
        or len(uploaded_files) != 1
    ):
        return {"error": "invalid_payload"}, 400

    uploaded_file = uploaded_files[0]
    filename = uploaded_file.filename
    if (
        not filename
        or len(filename) > 100
        or not filename.lower().endswith(".txt")
        or not filename.isprintable()
        or "/" in filename
        or "\\" in filename
    ):
        return {"error": "invalid_filename"}, 400

    uploaded_bytes = uploaded_file.read(MAX_TEXT_FILE_BYTES + 1)
    file_size_bytes = len(uploaded_bytes)
    if file_size_bytes > MAX_TEXT_FILE_BYTES:
        return {"error": "file_too_large"}, 413
    try:
        file_text = uploaded_bytes.decode("utf-8-sig")
    except UnicodeDecodeError:
        return {"error": "invalid_text_encoding"}, 400

    reported_hostname = form_fields["hostname"]
    if (
        not 1 <= len(reported_hostname) <= 253
        or not reported_hostname.isprintable()
        or reported_hostname != reported_hostname.strip()
    ):
        return {"error": "invalid_hostname"}, 400

    submission_id = form_fields["submission_id"]
    if len(submission_id) != 36:
        return {"error": "invalid_submission_id"}, 400
    try:
        submission_id = str(UUID(submission_id))
    except ValueError:
        return {"error": "invalid_submission_id"}, 400

    loot_entry, loot_created = database.record_loot(
        current_app.config["DATABASE_PATH"],
        helper_id=helper_id,
        submission_id=submission_id,
        reported_hostname=reported_hostname,
        observed_peer_ip=request.remote_addr,
        file_text=file_text,
        filename=filename,
        file_size_bytes=file_size_bytes,
    )
    if (
        loot_entry["reported_hostname"] != reported_hostname
        or loot_entry["file_text"] != file_text
        or loot_entry["filename"] != filename
        or loot_entry["file_size_bytes"] != file_size_bytes
    ):
        return {"error": "submission_id_conflict"}, 409
    return {"loot_id": loot_entry["loot_id"]}, 201 if loot_created else 200
