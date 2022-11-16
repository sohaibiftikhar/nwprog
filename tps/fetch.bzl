load("@//tps/liburing:fetch.bzl", liburing_fetch = "fetch")
load("@//tps/rules_foreign_cc:fetch.bzl", rules_foreign_cc_fetch = "fetch")

def fetch():
    """Fetch all third party deps needed."""
    rules_foreign_cc_fetch()
    liburing_fetch()
