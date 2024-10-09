def inja_template(name, src, base_dir, data = [], **kwargs):
    native.genrule(
        name = name,
        srcs = [src, base_dir] + data,
        outs = [name],
        cmd = "./$(execpath //tools:compile_template_main) --input=$(rootpath " +
              src + ") --output=$@ --base_dir=$$(dirname $(rootpath " + base_dir + "))/",
        tools = [
            "//tools:compile_template_main",
        ],
    )
