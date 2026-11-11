from flask import Blueprint, abort, current_app, render_template, request

from . import database

# Local, read-only operator dashboard.
dashboard_blueprint = Blueprint("dashboard", __name__)


@dashboard_blueprint.before_app_request
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
