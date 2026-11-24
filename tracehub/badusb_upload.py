from io import BytesIO
from hashlib import sha256
import mimetypes
import os
from tempfile import NamedTemporaryFile
from uuid import UUID, uuid4
from pathlib import PurePosixPath
from zipfile import BadZipFile, ZipFile

from flask import Blueprint, abort, current_app, render_template, request

from . import database

# Browser upload for the fixed BadUSB demo on the isolated LAN.
badusb_upload_blueprint = Blueprint("badusb_upload", __name__)


@badusb_upload_blueprint.get("/badusb/upload")
def show_badusb_upload():
    return (
        render_template("badusb_upload.html", submission_id=str(uuid4())),
        200,
        {"Cache-Control": "no-store"},
    )


@badusb_upload_blueprint.post("/badusb/upload")
def receive_badusb_upload():
    submission_ids = request.form.getlist("submission_id")
    uploaded_files = request.files.getlist("file")
    if len(submission_ids) != 1 or len(uploaded_files) != 1:
        abort(400, "Upload one Test_Important.zip file.")

    try:
        submission_id = str(UUID(submission_ids[0]))
    except ValueError:
        abort(400, "Open a fresh BadUSB upload page and try again.")

    uploaded_file = uploaded_files[0]
    if uploaded_file.filename != "Test_Important.zip":
        abort(400, "Choose Test_Important.zip.")

    # Read entries in memory. Bound expanded data before decompressing it.
    archive_bytes = uploaded_file.read()
    parts = []
    try:
        with ZipFile(BytesIO(archive_bytes)) as archive:
            entries = archive.infolist()
            if len(entries) > 100 or sum(entry.file_size for entry in entries) > 16 * 1024 * 1024:
                abort(413, "ZIP limit is 100 entries and 16 MiB of expanded data.")
            for entry in sorted(entries, key=lambda item: item.filename):
                path = PurePosixPath(entry.filename)
                if (
                    not path.parts or path.parts[0] != "Test_Important"
                    or ".." in path.parts or "\\" in entry.filename
                    or not entry.filename.isprintable() or len(entry.filename) > 500
                ):
                    abort(400, "ZIP entries must be inside Test_Important.")
                if entry.is_dir():
                    continue
                uploaded_bytes = archive.read(entry)
                media_type = mimetypes.guess_type(entry.filename)[0] or ""
                file_text = f"[Document, {len(uploaded_bytes)} bytes. Download the ZIP to open it.]"
                if media_type.startswith("text/") or path.suffix.lower() in {".json", ".log", ".md", ".yaml", ".yml"}:
                    try:
                        encoding = "utf-16" if uploaded_bytes.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
                        file_text = uploaded_bytes.decode(encoding)
                    except UnicodeDecodeError:
                        file_text = f"[Text encoding unavailable, {len(uploaded_bytes)} bytes. Download the ZIP to open it.]"
                parts.append((entry.filename, file_text))
    except (BadZipFile, RuntimeError, NotImplementedError):
        abort(400, "Upload a valid, unencrypted ZIP archive.")
    if not parts:
        abort(400, "Test_Important.zip must contain a file.")

    manifest = "\n\n".join(f"### {path}\n{text}" for path, text in parts)

    # Save the original ZIP atomically before acknowledging its database record.
    archive_sha256 = sha256(archive_bytes).hexdigest()
    archive_directory = current_app.config["DATABASE_PATH"].parent / "archives"
    archive_directory.mkdir(parents=True, exist_ok=True)
    with NamedTemporaryFile(dir=archive_directory, suffix=".tmp", delete=False) as temporary_archive:
        temporary_path = temporary_archive.name
        try:
            temporary_archive.write(archive_bytes)
            temporary_archive.flush()
            os.fsync(temporary_archive.fileno())
        except BaseException:
            temporary_archive.close()
            os.unlink(temporary_path)
            raise
    try:
        os.link(temporary_path, archive_directory / f"{archive_sha256}.zip")
    except FileExistsError:
        # Identical retries share the immutable original archive.
        pass
    finally:
        if os.path.exists(temporary_path):
            os.unlink(temporary_path)

    loot_entry, loot_created = database.record_loot(
        current_app.config["DATABASE_PATH"],
        helper_id="",
        submission_id=submission_id,
        reported_hostname="",
        observed_peer_ip=request.remote_addr,
        file_text=manifest,
        filename=uploaded_file.filename,
        file_size_bytes=len(archive_bytes),
        archive_sha256=archive_sha256,
    )
    if (
        loot_entry["file_text"] != manifest
        or loot_entry["filename"] != uploaded_file.filename
        or loot_entry["file_size_bytes"] != len(archive_bytes)
        or loot_entry["archive_sha256"] != archive_sha256
    ):
        abort(409, "This form already saved a different upload. Open a fresh form.")

    if request.accept_mimetypes.best == "application/json":
        return {"loot_id": loot_entry["loot_id"]}, 201 if loot_created else 200

    return (
        render_template("badusb_upload.html", loot_id=loot_entry["loot_id"]),
        201 if loot_created else 200,
    )
