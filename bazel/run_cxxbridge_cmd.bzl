"""
The run_cxxbridge_cmd rule is a variant of the run_binary rule defined in
`@bazel_skylib//rules:run_binary.bzl` that specifically runs `@cxxbridge_cmd//:cxxbridge` and
provides a `crate_features` attribute that gets expanded into `--cfg` arguments to cxxbuild.

Defining this as a rule (rather than a macro that thinly wraps `run_binary`) allows `crate_features`
to be evaluated as a "select" [1], which is not possible for macro parameters [2].

[1] https://bazel.build/docs/configurable-attributes#select-and-dependencies
[2] https://bazel.build/docs/configurable-attributes#faq-select-macro
"""

def _run_cxxbridge_cmd_impl(ctx):
    tool_as_list = [ctx.attr._cxxbridge]
    args = [
        # https://bazel.build/rules/lib/builtins/ctx#expand_location
        ctx.expand_location(a, tool_as_list)
        for a in ctx.attr.args
    ]
    for f in ctx.attr.crate_features:
        args.append("--cfg")
        args.append("feature=\"%s\"" % f)

    # https://bazel.build/rules/lib/builtins/ctx#resolve_tools
    tool_inputs, tool_input_manifests = ctx.resolve_tools(tools = tool_as_list)

    # https://bazel.build/rules/lib/builtins/actions.html#run
    ctx.actions.run(
        outputs = ctx.outputs.outs,
        inputs = ctx.files.srcs,
        tools = tool_inputs,
        executable = ctx.executable._cxxbridge,
        arguments = args,
        mnemonic = "RunCxxbridgeCmd",
        input_manifests = tool_input_manifests,
    )

    return DefaultInfo(
        files = depset(ctx.outputs.outs),
        runfiles = ctx.runfiles(ctx.outputs.outs),
    )

run_cxxbridge_cmd = rule(
    implementation = _run_cxxbridge_cmd_impl,
    attrs = {
        "srcs": attr.label_list(
            doc = "Source dependencies for this rule",
            allow_files = True,
            mandatory = True,
        ),
        "outs": attr.output_list(
            doc = "C++ output files generated by cxxbridge_cmd.",
            mandatory = True,
        ),
        "args": attr.string_list(
            doc = "Arguments to `cxxbridge_cmd`.",
            mandatory = True,
        ),
        "crate_features": attr.string_list(
            doc = "Optional list of cargo features that CXX bridge definitions may depend on.",
        ),
        "_cxxbridge": attr.label(
            default = Label("@cxxbridge_cmd//:cxxbridge"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
