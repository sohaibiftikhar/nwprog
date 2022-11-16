load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

def setup():
    """Setup rules foreign cpp build rules."""

    rules_foreign_cc_dependencies()
