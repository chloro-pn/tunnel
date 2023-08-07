load('@bazel_tools//tools/build_defs/repo:git.bzl', 'git_repository', 'new_git_repository')
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "bazel_skylib",
    sha256 = "66ffd9315665bfaafc96b52278f57c7e2dd09f5ede279ea6d39b2be471e7e3aa",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

git_repository(
    name = "googletest",
    remote = "https://ghproxy.com/https://github.com/google/googletest",
    tag = "release-1.11.0",
)

git_repository(
    name = "async_simple",
    remote = "https://github.com/alibaba/async_simple",
    commit = "584e1f1c8d69f1ebd826c674beea0dababdd9ae0",
)

git_repository(
    name = "gflags",
    remote = "https://github.com/gflags/gflags",
    tag = "v2.2.2",
)

new_git_repository(
    name = "mpmcqueue",
    remote = "https://github.com/rigtorp/MPMCQueue",
    commit = "28d05c021d68fc5280b593329d1982ed02f9d7b3",
    build_file = "//third_party:mpmc_queue.build",
)

new_git_repository(
    name = "asio",
    remote = "https://github.com/chriskohlhoff/asio",
    tag = "asio-1-28-0",
    build_file = "//third_party:asio.build",
)
