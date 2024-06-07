import json
import os
import re
import textwrap
from typing import Any

from absl import app
from absl import flags

FLAGS = flags.FLAGS

flags.DEFINE_string("input", None, "input json")
flags.mark_flag_as_required("input")

flags.DEFINE_string("output_dir", None, "output directory")
flags.mark_flag_as_required("output_dir")

flags.DEFINE_bool("overwrite", False, "whether or not to overwrite")

_ENUM_LIST = ("attr", "supportUnit", "unit")

_INT32_MAX = 2**31 - 1
_INT64_MAX = 2**63 - 1


def singularize(name: str) -> str:
    lower_name = name.lower()
    if (
        lower_name.endswith("bonuses")
        or lower_name.endswith("boxes")
        or lower_name.endswith("classes")
        or lower_name.endswith("passes")
    ):
        return name[:-2]
    if (
        lower_name.endswith("stories")
        or lower_name.endswith("categories")
        or lower_name.endswith("penalties")
        or lower_name.endswith("difficulties")
        or lower_name.endswith("lobbies")
        or lower_name.endswith("summaries")
        or lower_name.endswith("facilities")
        or lower_name.endswith("accessories")
        or lower_name.endswith("rarities")
    ):
        return name[:-3] + "y"
    if lower_name.endswith("s"):
        return name[:-1]
    return name


def camel_to_snake(name: str) -> str:
    name = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
    return re.sub("([a-z0-9])([A-Z])", r"\1_\2", name).lower()


def snake_to_pascal(name: str) -> str:
    return "".join(word.title() for word in name.split("_"))


def merge_list_val(
    key: str, vals: list[Any], schema: dict[str, Any], repeated: bool = True
):
    if repeated:
        schema[f"__repeated_{key}"] = {True}
    if not vals:
        return
    if isinstance(vals[0], dict):
        for val in vals:
            merge_dict_val(key, val, schema)
        return
    if key not in schema:
        schema[key] = set(vals)
    else:
        schema[key] |= set(vals)


def merge_dict_val(key: str, obj: dict[str, Any], schema: dict[str, Any]):
    if key not in schema:
        schema[key] = {}
    merge_schema(obj, schema[key])


def merge_plain_val(key: str, val: Any, schema: dict[str, Any]):
    merge_list_val(key, [val], schema, repeated=False)


def merge_schema(obj: dict[str, Any], schema: dict[str, Any]):
    for key, val in obj.items():
        if isinstance(val, list):
            merge_list_val(key, val, schema)
        elif isinstance(val, dict):
            merge_dict_val(key, val, schema)
        else:
            merge_plain_val(key, val, schema)


def guess_type(key: str, vals: set[Any]) -> str | set[Any]:
    val = list(vals)[0]
    if key.endswith("Type") or key in _ENUM_LIST:
        return list(vals)
    if isinstance(val, bool):
        return "bool"
    if isinstance(val, int):
        max_val = max(vals)
        if max_val <= _INT32_MAX:
            return "int32"
        if max_val <= _INT64_MAX:
            return "int64"
        return "uint64"
    if isinstance(val, str):
        return "string"
    if isinstance(val, float):
        return "float"
    raise RuntimeError("unhandled type case")


def proto_from_schema(schema: dict[str, Any]) -> dict[str, Any]:
    defn = {}
    for key, vals in schema.items():
        if isinstance(vals, dict):
            defn[key] = proto_from_schema(vals)
        else:
            try:
                defn[key] = guess_type(key, vals)
            except TypeError as e:
                print(key, vals)
                raise e
    return defn


def create_def(
    def_name: str, schema: dict[str, Any] | list[str], filename: str | None = None
) -> str:
    field_num = 1
    fields = []
    subfields = {}
    if isinstance(schema, dict):
        def_spec = "message"
        for key, val in schema.items():
            name = camel_to_snake(key)
            if key.startswith("__repeated"):
                continue
            if f"__repeated_{key}" in schema:
                spec = "repeated"
            else:
                spec = "optional"
            if isinstance(val, dict) or isinstance(val, list):
                val_type = singularize(snake_to_pascal(name))
                subfields[val_type] = val
            else:
                val_type = val
            fields.append(
                f'{spec} {val_type} {name} = {field_num} [(json_name) = "{key}"];'
            )
            field_num += 1
    else:
        def_spec = "enum"
        unknown_name = camel_to_snake(def_name).upper()
        fields.append(f"UNKNOWN_{unknown_name} = 0;")
        for val in sorted(schema):
            val_name = val.upper()
            fields.append(f'{val_name} = {field_num} [(json_value) = "{val}"];')
            field_num += 1

    fields = "\n  ".join(fields)
    if filename:
        options = f'\n  option (master_db_file) = "{filename}";'
    else:
        options = ""
    if not subfields:
        return f"""\
{def_spec} {def_name} {{{options}
  {fields}
}}"""
    other_defs = []
    for key, val in subfields.items():
        other_defs.append(create_def(key, val))
    other_defs = textwrap.indent("\n\n".join(other_defs), prefix="  ")
    return f"""
{def_spec} {def_name} {{{options}
{other_defs}

  {fields}
}}"""


def main(argv):
    with open(FLAGS.input) as f:
        objs = json.loads(f.read())
    json_file_name = os.path.basename(FLAGS.input)
    file_base = singularize(camel_to_snake(os.path.splitext(json_file_name)[0]))
    def_name = snake_to_pascal(file_base)
    schema = {}
    if isinstance(objs, list):
        for obj in objs:
            merge_schema(obj, schema)
    else:
        merge_schema(objs, schema)
    proto_def = proto_from_schema(schema)
    file_name = file_base + ".proto"
    dst_path = os.path.join(FLAGS.output_dir, file_name)
    new_file = True
    if os.path.exists(dst_path):
        if not FLAGS.overwrite:
            print(
                f"Destination already exists: {dst_path}. Add --overwrite to overwrite."
            )
            return 0
        new_file = False
    with open(dst_path, "w") as f:
        f.write(
            textwrap.dedent(
                """\
                syntax = "proto3";

                package sekai.db;

                import "sekai/db/proto/descriptor.proto";
                """
            )
        )
        f.write(create_def(def_name, proto_def, json_file_name))
    if new_file:
        with open(os.path.join(FLAGS.output_dir, "BUILD"), "a") as f:
            f.write(
                textwrap.dedent(
                    f"""\
                    proto_library(
                        name = "{file_base}_proto",
                        srcs = ["{file_name}"],
                        deps = [":descriptor_proto"],
                    )
                    cc_proto_library(
                        name = "{file_base}_cc_proto",
                        deps = [":{file_base}_proto"],
                    )
                    """
                )
            )
    print(f"Written to: {dst_path}")


if __name__ == "__main__":
    app.run(main)
