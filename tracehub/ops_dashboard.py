from flask import Blueprint, abort, current_app, render_template, request, send_file

from . import database

# Local, read-only operator dashboard.
dashboard_blueprint = Blueprint("dashboard", __name__)


@dashboard_blueprint.get("/loot/<uuid:loot_id>/archive")
def download_loot_archive(loot_id):
    database_path = current_app.config["DATABASE_PATH"]
    archive = database.find_loot_archive(database_path, str(loot_id))
    if archive is None or archive[0] is None:
        abort(404)
    archive_path = database_path.parent / "archives" / f"{archive[0]}.zip"
    if not archive_path.is_file():
        abort(404)
    return send_file(archive_path, as_attachment=True, download_name="Test_Important.zip")


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
    return render_template(
        "ops_dashboard.html",
        inventory=inventory,
        latest=latest,
        matching=matching,
        loot_entries=entries,
        query=query,
        hostname_status=hostname_status,
        sort=sort,
    )
