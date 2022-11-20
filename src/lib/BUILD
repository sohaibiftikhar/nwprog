package(default_visibility = ["//visibility:public"])

cc_library(
    name = "result",
    srcs = ["result.inl"],
    hdrs = ["result.hh"],
    deps = [":concepts"],
)

cc_library(
    name = "concepts",
    hdrs = ["concepts.hh"],
)

cc_library(
    name = "function",
    hdrs = ["function.hh"],
)

cc_library(
    name = "log",
    srcs = ["log.cc"],
    hdrs = ["log.hh"],
)

cc_library(
    name = "scope_guard",
    srcs = [
        "scope_guard.inl",
    ],
    hdrs = [
        "scope_guard.hh",
    ],
)
