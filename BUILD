package(default_visibility = ["//visibility:public"])

cc_library(
  name = "tunnel",
  hdrs = glob(["include/**/*.h"]),
  srcs = glob(["src/*.cc"]),
  includes = ["include"],
  deps = [
    "@async_simple//:async_simple",
  ]
)

cc_test(
  name = "test",
  srcs = glob(["test/*.cc", "test/*.h"]),
  includes = ["test"],
  deps = [
    ":tunnel",
    "@async_simple//:simple_executors",
    "@googletest//:gtest",
    "@googletest//:gtest_main",
  ]
)

cc_binary(
  name = "example",
  srcs = glob(["example/*.cc"]),
  deps = [
    ":tunnel",
    "@async_simple//:simple_executors",
  ],
)