package(default_visibility = ["//visibility:public"])

config_setting(
  name = "msvc",
  flag_values = {
    "@bazel_tools//tools/cpp:compiler": "msvc-cl",
  },
)

TUNNEL_COPTS = select({
  ":msvc" : [
    "/std:c++20",
    "/await:strict",
    "/EHa",
  ],
  "//conditions:default" : [
    "-std=c++20",
    "-D_GLIBCXX_USE_CXX11_ABI=1",
    "-Wno-deprecated-register",
    "-Wno-mismatched-new-delete",
    "-Wall",
    "-g",
  ],
})

cc_library(
  name = "tunnel",
  hdrs = glob(["include/**/*.h"]),
  includes = ["include"],
  copts = TUNNEL_COPTS,
  deps = [
    "@async_simple//:async_simple",
    "@mpmcqueue//:mpmc_queue",
    "@asio//:asio",
    "@spdlog//:spdlog",
  ]
)

cc_test(
  name = "test",
  srcs = glob(["test/*.cc", "test/*.h"]),
  includes = ["test"],
  copts = TUNNEL_COPTS,
  deps = [
    ":tunnel",
    "@async_simple//:simple_executors",
    "@googletest//:gtest",
    "@googletest//:gtest_main",
  ]
)

cc_binary(
  name = "hello_world",
  srcs = glob(["example/hello_world.cc"]),
  copts = TUNNEL_COPTS,
  deps = [
    ":tunnel",
    "@async_simple//:simple_executors",
  ],
)

cc_binary(
  name = "number_handle",
  srcs = glob(["example/number_handle.cc"]),
  copts = TUNNEL_COPTS,
  deps = [
    ":tunnel",
    "@async_simple//:simple_executors",
  ],
)


cc_binary(
  name = "embed_pipeline",
  srcs = glob(["example/embed_pipeline.cc"]),
  copts = TUNNEL_COPTS,
  deps = [
    ":tunnel",
    "@async_simple//:simple_executors",
  ],
)

cc_binary(
  name = "switcher_server",
  srcs = glob(["src/*.cc"]),
  copts = TUNNEL_COPTS,
  deps = [
    ":tunnel",
    "@asio//:asio",
    "@gflags//:gflags",
  ],
)