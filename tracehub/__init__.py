import os
from pathlib import Path
from flask import Flask
from . import database
from .badusb_upload import badusb_upload_blueprint
from .loot_receiver import load_helper_tokens, loot_receiver_blueprint
from .ops_dashboard import dashboard_blueprint


def create_app(server_role="dashboard"):
    instance_directory = Path(os.environ["LOCALAPPDATA"]) / "TraceDeck"
    app = Flask(__name__, instance_path=str(instance_directory), static_folder=None)
    app.config["DATABASE_PATH"] = instance_directory / "tracehub.sqlite3"

    if server_role == "dashboard":
        app.config["TRUSTED_HOSTS"] = ["127.0.0.1"]
        app.register_blueprint(dashboard_blueprint)
    elif server_role == "loot_receiver":
        app.config["HELPER_TOKENS"] = load_helper_tokens(instance_directory)
        app.config["MAX_CONTENT_LENGTH"] = 4096
        app.register_blueprint(loot_receiver_blueprint)
    elif server_role == "badusb_demo":
        app.config["MAX_CONTENT_LENGTH"] = 4096
        app.register_blueprint(badusb_upload_blueprint)
    else:
        raise ValueError("Choose the 'dashboard', 'loot_receiver', or 'badusb_demo' role.")

    database.initialize_database(app.config["DATABASE_PATH"])
    return app
