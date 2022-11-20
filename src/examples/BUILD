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
    deps = ["@liburing"],
)

cc_binary(
    name = "echo_server",
    srcs = [
        "echo_server.cc",
    ],
    deps = [
        "//src/lib:log",
        "//src/lib:scope_guard",
        "@liburing",
    ],
)
