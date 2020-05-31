#!/usr/bin/env python3
import argparse
import jinja2
from glob import glob
from lark import Lark

next_msg_id = 1
class IDLParser:
    def __init__(self):
        self.msgs = []

    def parse(self, path):
        parser = Lark("""
            %import common.WS
            %ignore WS
            %ignore COMMENT
            COMMENT: /\/\/.*/
            NAME: /[a-zA-Z_][a-zA-Z_0-9]*/
            PATH: /"[^"]+"/
            NATINT: /[1-9][0-9]*/

            start: stmts?
            stmts: stmt*
            stmt: msg_def
                | namespace_def
                | include_stmt

            include_stmt: "include" PATH ";"
            namespace_def: "namespace" NAME "{" stmts "}"
            msg_def: modifiers? NAME "(" fields ")" ["->" "(" fields ")"] ";"
            modifiers: MODIFIER*
            MODIFIER: "rpc" | "oneway"
            fields: (field ("," field)*)?
            field: NAME ":" type
            type: NAME ("[" NATINT "]")?
        """)

        ast = parser.parse(open(path).read())
        for stmt in ast.children[0].children:
            self.visit_stmt("", stmt)

        return {
            "msgs": self.msgs,
        }

    def visit_stmt(self, namespace, tree):
        assert tree.data == "stmt"
        if tree.children[0].data == "namespace_def":
            for stmt in tree.children[0].children[1].children:
                namespace = tree.children[0].children[0].value
                self.visit_stmt(namespace, stmt)
        elif tree.children[0].data == "msg_def":
            msg_def = self.visit_msg_def(namespace, tree.children[0])
            self.msgs.append(msg_def)
        elif tree.children[0].data == "include_stmt":
            pattern = tree.children[0].children[0].value.strip('"')
            for path in glob(pattern):
                idl = IDLParser().parse(path)
                self.msgs += idl["msgs"]
        else:
            # unreachable
            assert False

    def is_bulk_field_type(self, typename):
        bulk_builtins = ["str", "bytes"]
        return typename in bulk_builtins

    def visit_msg_def(self, namespace, tree):
        global next_msg_id
        assert tree.data == "msg_def"
        modifiers = list(map(lambda x: x.value, tree.children[0].children))
        name = tree.children[1].value
        args = self.visit_fields(tree.children[2].children)
        args_id = next_msg_id
        next_msg_id += 1

        if len(tree.children) > 3:
            rets = self.visit_fields(tree.children[3].children)
            rets_id = next_msg_id
            next_msg_id += 1
        else:
            rets = []
            rets_id = None

        msg_def = {
            "args_id": args_id,
            "rets_id": rets_id,
            "name": name,
            "namespace": namespace,
            "oneway": "oneway" in modifiers,
            "args": args,
            "rets": rets,
        }

        return msg_def

    def visit_fields(self, trees):
        bulk_field = None
        inline_fields = []
        for tree in trees:
            field = self.visit_field(tree)
            if self.is_bulk_field_type(field["type"]["name"]):
                if bulk_field:
                    raise Exception(f"{name}: multiple bulk fields are not allowed: {bulk_field['name']}, {field['name']}")
                bulk_field = field
                bulk_field["is_str"] = field["type"]["name"] == "str"
            else:
                inline_fields.append(field)

        return {
            "inlines": inline_fields,
            "bulk": bulk_field,
        }

    def visit_field(self, tree):
        assert tree.data == "field"
        return {
            "name": tree.children[0].value,
            "type": self.visit_type(tree.children[1]),
        }

    def visit_type(self, tree):
        assert tree.data == "type"
        if len(tree.children) > 1:
            nr = int(tree.children[1].value)
        else:
            nr = None

        return {
            "name": tree.children[0].value,
            "nr": nr,
        }

def c_generator(args, idl):
    builtins = dict(
        str="const char *",
        bytes="const void *",
        char="char",
        int="int",
        uint="unsigned",
        i8="int8_t",
        i16="int16_t",
        i32="int32_t",
        i64="int64_t",
        u8="uint8_t",
        u16="uint16_t",
        u32="uint32_t",
        u64="uint64_t",
        size="size_t",
        exception_type="enum exception_type",
    )

    def msg_type(fields):
        flags = ""
        if fields["bulk"]:
            flags += "| MSG_BULK"
            if fields["bulk"]["is_str"]:
                flags += "| MSG_STR"
        return flags

    def field_def(field):
        type_ = field["type"]
        def_ = builtins.get(type_["name"], f"{type_['name']}_t")
        def_ += f" {field['name']}"
        if type_["nr"]:
            def_ += f"[{type_['nr']}]"
        return def_

    renderer = jinja2.Environment()
    renderer.filters["msg_name"] = lambda m: f"{m['namespace']}_".lstrip("_") + m['name']
    renderer.filters["field_def"] = field_def
    renderer.filters["msg_type"] = msg_type
    template = renderer.from_string ("""\
#ifndef __GENIDL_MESSAGE_H__
#define __GENIDL_MESSAGE_H__
#include <types.h>

{% for msg in msgs %}
struct {{ msg | msg_name }}_fields {{ "{" }}
{%- if msg.args.bulk %}
    {%- if msg.args.bulk.is_str %}
    const char *{{ msg.args.bulk.name }};
    {% else %}
    const void *{{ msg.args.bulk.name }};
    {% endif %}
    size_t {{ msg.args.bulk.name }}_len;
{% endif %}
{%- for field in msg.args.inlines %}
    {{ field | field_def }};
{%- endfor %}
{{ "};" }}

{%- if not msg.oneway %}
struct {{ msg | msg_name }}_reply_fields {{ "{" }}
{%- if msg.rets.bulk %}
    {%- if msg.rets.bulk.is_str %}
    const char *{{ msg.rets.bulk.name }};
    {% else %}
    const void *{{ msg.rets.bulk.name }};
    {% endif %}
    size_t {{ msg.rets.bulk.name }}_len;
{% endif %}
{%- for field in msg.rets.inlines %}
    {{ field | field_def }};
{%- endfor %}
{{ "};" }}
{%- endif %}
{% endfor %}

{%- for msg in msgs %}
#define {{ msg | msg_name | upper }}_MSG ({{ msg.args_id }}{{ msg.args | msg_type }})
{%- if not msg.oneway %}
#define {{ msg | msg_name | upper }}_REPLY_MSG ({{ msg.rets_id }}{{ msg.rets | msg_type }})
{%- endif %}
{%- endfor %}

#define IDL_MESSAGE_FIELDS \\
{%- for msg in msgs %}
    struct {{ msg | msg_name }}_fields {{ msg | msg_name }}; \\
{%- if not msg.oneway %}
    struct {{ msg | msg_name }}_reply_fields {{ msg | msg_name }}_reply; \\
{%- endif %}
{%- endfor %}

#define IDL_STATIC_ASSERTS \\
{%- for msg in msgs %}
{%- if msg.args.bulk %}
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}.{{ msg.args.bulk.name }}) == offsetof(struct message, bulk_ptr)); \\
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}.{{ msg.args.bulk.name }}_len) == offsetof(struct message, bulk_len)); \\
{%- endif %}
{%- if not msg.oneway %}
{%- if msg.rets.bulk %}
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}_reply.{{ msg.rets.bulk.name }}) == offsetof(struct message, bulk_ptr)); \\
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}_reply.{{ msg.rets.bulk.name }}_len) == offsetof(struct message, bulk_len)); \\
{%- endif %}
{%- endif %}
{%- endfor %}

#endif

""")

    text = template.render(msgs=idl["msgs"])

    with open(args.out, "w") as f:
        f.write(text)

def main():
    parser = argparse.ArgumentParser(
        description="The message definitions generator.")
    parser.add_argument("--idl", required=True, help="The IDL file.")
    parser.add_argument("--lang", choices=["c"], default="c",
        help="The output language.")
    parser.add_argument("-o", dest="out", required=True,
        help="The output directory.")
    args = parser.parse_args()

    idl = IDLParser().parse(args.idl)
    if args.lang == "c":
        c_generator(args, idl)

if __name__ == "__main__":
    main()
