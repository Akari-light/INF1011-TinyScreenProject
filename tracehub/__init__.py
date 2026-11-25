import os
from pathlib import Path

from flask import Flask

from . import database
from .badusb_upload import badusb_upload_blueprint
from .loot_receiver import load_helper_tokens, loot_receiver_blueprint
from .dashboard.views import dashboard_blueprint
from .reverse_shell import reverse_shell_blueprint, start_reverse_shell_listener
from .walk.views import walk_blueprint


def create_app(server_role="dashboard"):
    # Create one TraceHub listener using the selected server role.
    instance_directory = Path(os.environ["LOCALAPPDATA"]) / "TraceDeck"
    app = Flask(
        __name__,
        instance_path=str(instance_directory),
        static_folder="static" if server_role in ("dashboard", "demo", "walk") else None,
    )
    app.config["DATABASE_PATH"] = instance_directory / "tracehub.sqlite3"
    if server_role in ("dashboard", "demo", "walk"):
        app.register_blueprint(walk_blueprint)

    if server_role == "dashboard":
        app.config["TRUSTED_HOSTS"] = ["127.0.0.1"]
        app.config["SHELL_HOST"] = os.environ.get("TRACEDECK_SHELL_HOST", "0.0.0.0")
        app.config["SHELL_PORT"] = int(os.environ.get("TRACEDECK_SHELL_PORT", "4444"))
        app.register_blueprint(dashboard_blueprint)
        app.register_blueprint(reverse_shell_blueprint)
        start_reverse_shell_listener(app.config["SHELL_HOST"], app.config["SHELL_PORT"])
    elif server_role == "loot_receiver":
        app.config["HELPER_TOKENS"] = load_helper_tokens(instance_directory)
        app.config["MAX_CONTENT_LENGTH"] = 4096
        app.register_blueprint(loot_receiver_blueprint)
    elif server_role == "badusb_demo":
        app.config["MAX_CONTENT_LENGTH"] = 8 * 1024 * 1024
        app.register_blueprint(badusb_upload_blueprint)
    elif server_role == "demo":
        app.config["MAX_CONTENT_LENGTH"] = 8 * 1024 * 1024
        app.config["SHELL_HOST"] = os.environ.get("TRACEDECK_SHELL_HOST", "0.0.0.0")
        app.config["SHELL_PORT"] = int(os.environ.get("TRACEDECK_SHELL_PORT", "4444"))
        app.register_blueprint(dashboard_blueprint)
        app.register_blueprint(badusb_upload_blueprint)
        app.register_blueprint(reverse_shell_blueprint)
        start_reverse_shell_listener(app.config["SHELL_HOST"], app.config["SHELL_PORT"])
    elif server_role == "walk":
        app.config["TRUSTED_HOSTS"] = ["127.0.0.1", "localhost"]
        app.add_url_rule("/", "walk_home", app.view_functions["walk.trials"])
    else:
        raise ValueError(f"Unknown TraceHub role: {server_role}")

    database.initialize_database(app.config["DATABASE_PATH"])
    return app
