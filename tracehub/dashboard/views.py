from io import BytesIO
from pathlib import PurePosixPath
from zipfile import BadZipFile, ZipFile

from flask import Blueprint, abort, current_app, render_template, request, send_file

from .. import database

# Local, read-only operator dashboard.
dashboard_blueprint = Blueprint("dashboard", __name__)
IMAGE_TYPES = {
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".png": "image/png",
    ".gif": "image/gif",
    ".webp": "image/webp",
}


@dashboard_blueprint.get("/loot/<uuid:loot_id>/image/<int:entry_index>")
def show_loot_image(loot_id, entry_index):
    database_path = current_app.config["DATABASE_PATH"]
    archive_record = database.find_loot_archive(database_path, str(loot_id))
    if archive_record is None or archive_record[0] is None:
        abort(404)
    archive_path = database_path.parent / "archives" / f"{archive_record[0]}{database.archive_suffix(archive_record[1])}"
    try:
        with ZipFile(archive_path) as archive:
            entries = archive.infolist()
            if entry_index >= len(entries):
                abort(404)
            entry = entries[entry_index]
            media_type = IMAGE_TYPES.get(PurePosixPath(entry.filename).suffix.lower())
            if media_type is None or entry.is_dir() or entry.file_size > 16 * 1024 * 1024:
                abort(404)
            image_data = archive.read(entry)
    except (OSError, BadZipFile, RuntimeError, NotImplementedError):
        abort(404)

    # Verify the format before serving archive bytes inline in the console.
    signatures = {
        "image/jpeg": image_data.startswith(b"\xff\xd8\xff"),
        "image/png": image_data.startswith(b"\x89PNG\r\n\x1a\n"),
        "image/gif": image_data.startswith((b"GIF87a", b"GIF89a")),
        "image/webp": image_data.startswith(b"RIFF") and image_data[8:12] == b"WEBP",
    }
    if not signatures[media_type]:
        abort(415)
    response = send_file(BytesIO(image_data), mimetype=media_type)
    response.headers["X-Content-Type-Options"] = "nosniff"
    response.headers["Content-Security-Policy"] = "default-src 'none'"
    return response


@dashboard_blueprint.get("/loot/<uuid:loot_id>/archive")
def download_loot_archive(loot_id):
    database_path = current_app.config["DATABASE_PATH"]
    archive = database.find_loot_archive(database_path, str(loot_id))
    if archive is None or archive[0] is None:
        abort(404)
    archive_path = database_path.parent / "archives" / f"{archive[0]}{database.archive_suffix(archive[1])}"
    if not archive_path.is_file():
        abort(404)
    return send_file(archive_path, as_attachment=True, download_name=archive[1] or "upload")


@dashboard_blueprint.before_request
def require_local_operator():
    if request.remote_addr != "127.0.0.1":
        abort(403)


@dashboard_blueprint.get("/")
def show_dashboard():
    query = request.args.get("q", "").strip()[:200]
    hostname_status = request.args.get("hostname_status", "")
    sort = request.args.get("sort", "newest")
    if hostname_status not in database.LOOT_HOSTNAME_FILTERS:
        hostname_status = ""
    sort = sort if sort in database.LOOT_SORT_ORDERS else "newest"

    inventory, latest, matching, entries = database.read_loot_log(
        current_app.config["DATABASE_PATH"],
        query=query,
        hostname_status=hostname_status,
        sort=sort,
    )
    image_entries = {}
    for entry in entries:
        if not entry["archive_sha256"]:
            continue
        archive_path = current_app.config["DATABASE_PATH"].parent / "archives" / f"{entry['archive_sha256']}{database.archive_suffix(entry['filename'])}"
        try:
            with ZipFile(archive_path) as archive:
                image_entries[entry["loot_id"]] = [
                    {"index": index, "name": item.filename}
                    for index, item in enumerate(archive.infolist())
                    if not item.is_dir() and PurePosixPath(item.filename).suffix.lower() in IMAGE_TYPES
                    and item.file_size <= 16 * 1024 * 1024
                ]
        except (OSError, BadZipFile):
            image_entries[entry["loot_id"]] = []
    return render_template(
        "dashboard/index.html",
        inventory=inventory,
        latest=latest,
        matching=matching,
        loot_entries=entries,
        image_entries=image_entries,
        query=query,
        hostname_status=hostname_status,
        sort=sort,
    )
