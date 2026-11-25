"""Keep the dashboard blueprint import available to existing bench tools."""

from .dashboard.views import dashboard_blueprint

__all__ = ["dashboard_blueprint"]
