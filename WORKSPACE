new_local_repository(
    name = "external",
    path = ".",
    build_file = "BUILD.external",
)

local_repository(
    name = "googletest",
    path = "external/googletest",
)

load(":configure_copts.bzl", "configure_compiler_copts")
configure_compiler_copts(
    name = "generated_compiler_config",
    variable_template = "//:variables.bzl.tpl",
)
