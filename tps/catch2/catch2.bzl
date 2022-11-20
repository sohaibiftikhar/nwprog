load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")

def fetch():
    """Fetch catch2 deps."""

    http_file(
        name = "catch2.BUILD",
        sha256 = "6a4c85a4452386f58e4f01cc8be96ceb0dd4f3dd6dcad7e7a715bea2219f7bd4",
        urls = [
            "https://raw.githubusercontent.com/catchorg/Catch2/v3.1.1/BUILD.bazel",
        ],
    )

    http_archive(
        name = "catch2",
        sha256 = "2106bccfec18c8ce673623d56780220e38527dd8f283ccba26aa4b8758737d0e",
        urls = [
            "https://github.com/catchorg/Catch2/archive/refs/tags/v3.1.1.tar.gz",
        ],
        strip_prefix = "Catch2-3.1.1/",
        build_file = "@catch2.BUILD//file:downloaded",
    )
