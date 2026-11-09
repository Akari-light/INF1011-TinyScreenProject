"""Local, read-only operator dashboard."""

from flask import Blueprint, abort, current_app, render_template, request

from . import database

ops = Blueprint("ops", __name__)


@ops.before_request
def require_local_operator():
    if request.remote_addr != "127.0.0.1":
        abort(403)


@ops.get("/")
def show_dashboard():
    receipts = database.list_receipts(current_app.config["DATABASE_PATH"])
    return render_template("ops_dashboard.html", receipts=receipts)
