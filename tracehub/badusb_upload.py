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

# Browser upload for the BadUSB demo on the isolated LAN.
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
        abort(400, "Upload one file.")

    try:
        submission_id = str(UUID(submission_ids[0]))
    except ValueError:
        abort(400, "Open a fresh BadUSB upload page and try again.")

    uploaded_file = uploaded_files[0]
    safe_name = PurePosixPath((uploaded_file.filename or "").replace("\\", "/")).name
    if not safe_name or not safe_name.isprintable() or len(safe_name) > 200:
        abort(400, "Upload a file with a simple, printable name.")

    uploaded_bytes = uploaded_file.read()
    if not uploaded_bytes:
        abort(400, "Upload a non-empty file.")

    parts = []
    if safe_name.lower().endswith(".zip"):
        # Read ZIP entries in memory. Bound expanded data before decompressing it.
        try:
            with ZipFile(BytesIO(uploaded_bytes)) as archive:
                entries = archive.infolist()
                if len(entries) > 100 or sum(entry.file_size for entry in entries) > 16 * 1024 * 1024:
                    abort(413, "ZIP limit is 100 entries and 16 MiB of expanded data.")
                for entry in sorted(entries, key=lambda item: item.filename):
                    path = PurePosixPath(entry.filename)
                    if (
                        not path.parts
                        or ".." in path.parts
                        or "\\" in entry.filename
                        or not entry.filename.isprintable()
                        or len(entry.filename) > 500
                    ):
                        abort(400, "ZIP entries have invalid paths.")
                    if entry.is_dir():
                        continue
                    entry_bytes = archive.read(entry)
                    media_type = mimetypes.guess_type(entry.filename)[0] or ""
                    file_text = f"[Document, {len(entry_bytes)} bytes. Download the archive to open it.]"
                    if media_type.startswith("text/") or path.suffix.lower() in {".json", ".log", ".md", ".yaml", ".yml", ".txt"}:
                        try:
                            encoding = "utf-16" if entry_bytes.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
                            file_text = entry_bytes.decode(encoding)
                        except UnicodeDecodeError:
                            file_text = f"[Text encoding unavailable, {len(entry_bytes)} bytes. Download the archive to open it.]"
                    parts.append((entry.filename, file_text))
        except (BadZipFile, RuntimeError, NotImplementedError):
            abort(400, "Upload a valid, unencrypted ZIP archive.")
        if not parts:
            abort(400, "ZIP archive must contain a file.")
    else:
        # Plain file (e.g. a .txt dump): decode it directly, no ZIP required.
        try:
            encoding = "utf-16" if uploaded_bytes.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
            file_text = uploaded_bytes.decode(encoding)
        except UnicodeDecodeError:
            file_text = f"[Text encoding unavailable, {len(uploaded_bytes)} bytes.]"
        parts.append((safe_name, file_text))

    manifest = "\n\n".join(f"### {path}\n{text}" for path, text in parts)

    # Save the original upload atomically before acknowledging its database record.
    archive_sha256 = sha256(uploaded_bytes).hexdigest()
    archive_directory = current_app.config["DATABASE_PATH"].parent / "archives"
    archive_directory.mkdir(parents=True, exist_ok=True)
    stored_suffix = database.archive_suffix(safe_name)
    with NamedTemporaryFile(dir=archive_directory, suffix=".tmp", delete=False) as temporary_archive:
        temporary_path = temporary_archive.name
        try:
            temporary_archive.write(uploaded_bytes)
            temporary_archive.flush()
            os.fsync(temporary_archive.fileno())
        except BaseException:
            temporary_archive.close()
            os.unlink(temporary_path)
            raise
    try:
        os.link(temporary_path, archive_directory / f"{archive_sha256}{stored_suffix}")
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
        filename=safe_name,
        file_size_bytes=len(uploaded_bytes),
        archive_sha256=archive_sha256,
    )
    if (
        loot_entry["file_text"] != manifest
        or loot_entry["filename"] != safe_name
        or loot_entry["file_size_bytes"] != len(uploaded_bytes)
        or loot_entry["archive_sha256"] != archive_sha256
    ):
        abort(409, "This form already saved a different upload. Open a fresh form.")

    if request.accept_mimetypes.best == "application/json":
        return {"loot_id": loot_entry["loot_id"]}, 201 if loot_created else 200

    return (
        render_template("badusb_upload.html", loot_id=loot_entry["loot_id"]),
        201 if loot_created else 200,
    )
