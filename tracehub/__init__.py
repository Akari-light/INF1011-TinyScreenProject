import os
from pathlib import Path

from flask import Flask

from . import database
from .loot_receiver import intake, load_helper_tokens
from .ops_dashboard import ops


def create_app(interface="ops"):
    instance_directory = Path(os.environ["LOCALAPPDATA"]) / "TraceDeck"
    app = Flask(__name__, instance_path=str(instance_directory), static_folder=None)
    app.config["DATABASE_PATH"] = instance_directory / "tracehub.sqlite3"

    if interface == "ops":
        app.config["TRUSTED_HOSTS"] = ["127.0.0.1"]
        app.register_blueprint(ops)
    elif interface == "intake":
        app.config["HELPER_TOKENS"] = load_helper_tokens(instance_directory)
        app.config["MAX_CONTENT_LENGTH"] = 2048
        app.register_blueprint(intake)
    else:
        raise ValueError("TraceHub interface must be 'ops' or 'intake'.")

    database.initialize_database(app.config["DATABASE_PATH"])
    return app
