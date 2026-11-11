from uuid import UUID, uuid4

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
    if len(submission_ids) != 1 or len(uploaded_files) != 1 or len(request.files) != 1:
        abort(400, "Submit one file using the BadUSB upload page.")

    try:
        submission_id = str(UUID(submission_ids[0]))
    except ValueError:
        abort(400, "Open a fresh BadUSB upload page and try again.")

    uploaded_file = uploaded_files[0]
    if uploaded_file.filename != "hello-world.txt":
        abort(400, "Choose hello-world.txt.")

    # Flask caps the entire request at 4 KiB before parsing the upload.
    uploaded_bytes = uploaded_file.read()
    file_size_bytes = len(uploaded_bytes)
    try:
        file_text = uploaded_bytes.decode("utf-8-sig")
    except UnicodeDecodeError:
        abort(400, "Save hello-world.txt as UTF-8.")
    if file_text not in ("hello world", "hello world\n", "hello world\r\n"):
        abort(400, "The file must contain only hello world.")

    loot_entry, loot_created = database.record_loot(
        current_app.config["DATABASE_PATH"],
        helper_id="",
        submission_id=submission_id,
        reported_hostname="",
        observed_peer_ip=request.remote_addr,
        file_text=file_text,
        filename=uploaded_file.filename,
        file_size_bytes=file_size_bytes,
    )
    if (
        loot_entry["file_text"] != file_text
        or loot_entry["file_size_bytes"] != file_size_bytes
    ):
        abort(409, "This form already saved a different upload. Open a fresh form.")

    return (
        render_template("badusb_upload.html", loot_id=loot_entry["loot_id"]),
        201 if loot_created else 200,
    )
