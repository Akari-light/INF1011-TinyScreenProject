from flask import Blueprint, abort, current_app, render_template, request

from .walk_trials import evaluate, recent

walk_blueprint = Blueprint("walk", __name__)


@walk_blueprint.get("/walk-away")
def trials():
    if request.remote_addr != "127.0.0.1":
        abort(403)
    records = [(t, evaluate(t)) for t in recent(current_app.config["DATABASE_PATH"])]
    return render_template("walk_trials.html", records=records,
                           has_home="dashboard" in current_app.blueprints)
