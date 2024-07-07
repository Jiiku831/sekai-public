workspace(name = "sekai-public")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")

http_file(
    name = "sekai-best-music-metas",
    downloaded_file_path = "music_metas.json",
    url = "https://storage.sekai.best/sekai-best-assets/music_metas.json",
)

http_archive(
    name = "emsdk",
    sha256 = "6b206e135ccc3b0d6c02e948eb4b8b95521593b5b4d9788795269bbf6640fcb2",
    strip_prefix = "emsdk-4e2496141eda15040c44e9bbf237a1326368e34c/bazel",
    url = "https://github.com/emscripten-core/emsdk/archive/4e2496141eda15040c44e9bbf237a1326368e34c.tar.gz",
)

load("@emsdk//:deps.bzl", emsdk_deps = "deps")

emsdk_deps()

load("@emsdk//:emscripten_deps.bzl", emsdk_emscripten_deps = "emscripten_deps")

emsdk_emscripten_deps(emscripten_version = "3.1.51")

load("@emsdk//:toolchains.bzl", "register_emscripten_toolchains")

register_emscripten_toolchains()
