cc_binary(
    name = "dead_fd",
    srcs = [
        "dead_fd.cc",
    ],
    deps = [],
)

cc_binary(
    name = "event_fd",
    srcs = [
        "event_fd.cc",
    ],
    linkopts = ["-lpthread"],
    deps = ["@liburing"],
)

cc_binary(
    name = "kernel_side_polling",
    srcs = [
        "kernel_side_polling.cc",
    ],
    # linkopts = ["-lpthread"],
    deps = ["@liburing"],
)
