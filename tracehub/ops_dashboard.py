from flask import Blueprint, abort, current_app, render_template, request
from . import database

# Local, read-only operator dashboard.
dashboard_blueprint = Blueprint("dashboard", __name__)


@dashboard_blueprint.before_request
def require_local_operator():
    if request.remote_addr != "127.0.0.1":
        abort(403)


@dashboard_blueprint.get("/")
def show_dashboard():
    loot_entries = database.list_loot(current_app.config["DATABASE_PATH"])
    return render_template("ops_dashboard.html", loot_entries=loot_entries)
