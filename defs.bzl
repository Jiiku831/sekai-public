def inja_template(name, out, src, base_dir, blaze_out_base_dir, data = [], **kwargs):
    native.genrule(
        name = name,
        srcs = [src, base_dir, blaze_out_base_dir] + data,
        outs = [out],
        cmd = "./$(execpath //tools:compile_template_main) --input=$(execpath " +
              src + ") --output=$@ --base_dir=$$(dirname $(execpath " + base_dir + "))/ " +
              " --blaze_out_base_dir=$$(dirname $(execpath " + blaze_out_base_dir + "))/",
        tools = [
            "//tools:compile_template_main",
        ],
    )
