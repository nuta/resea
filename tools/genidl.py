#!/usr/bin/env python3
import argparse
import jinja2
from glob import glob
from lark import Lark

next_msg_id = 1
class IDLParser:
    def __init__(self):
        self.msgs = []
        self.typedefs = []
        self.consts = []
        self.enums = []

    def parse(self, path):
        parser = Lark("""
            %import common.WS
            %ignore WS
            %ignore COMMENT
            COMMENT: /\/\/.*/
            NAME: /[a-zA-Z_][a-zA-Z_0-9]*/
            PATH: /"[^"]+"/
            NATINT: /[1-9][0-9]*/
            CONST: /(0x[0-9]+)|([1-9][0-9]*)|0/

            start: stmts?
            stmts: stmt*
            stmt: msg_def
                | namespace_def
                | include_stmt
                | enum_stmt
                | type_stmt
                | const_stmt

            type_stmt: "type" NAME "=" type ";"
            const_stmt: "const" NAME ":" type "=" CONST ";"
            enum_stmt: "enum" NAME "{" enum_items "}" ";"
            enum_items: (enum_item ("," enum_item)*)? ","?
            enum_item: NAME "=" CONST
            include_stmt: "include" PATH ";"
            namespace_def: "namespace" NAME "{" stmts "}"
            msg_def: modifiers? NAME "(" fields ")" ["->" "(" fields ")"] ";"
            modifiers: MODIFIER*
            MODIFIER: "rpc" | "oneway" | "async"
            fields: (field ("," field)*)?
            field: NAME ":" type
            type: NAME ("[" NATINT "]")?
        """)

        ast = parser.parse(open(path).read())
        for stmt in ast.children[0].children:
            self.visit_stmt("", stmt)

        return {
            "msgs": self.msgs,
            "consts": self.consts,
            "typedefs": self.typedefs,
            "enums": self.enums,
        }

    def visit_stmt(self, namespace, tree):
        assert tree.data == "stmt"
        if tree.children[0].data == "namespace_def":
            name = tree.children[0].children[0].value
            for stmt in tree.children[0].children[1].children:
                self.visit_stmt(name, stmt)
        elif tree.children[0].data == "msg_def":
            msg_def = self.visit_msg_def(tree.children[0])
            msg_def["namespace"] = namespace
            self.msgs.append(msg_def)
        elif tree.children[0].data == "include_stmt":
            pattern = tree.children[0].children[0].value.strip('"')
            for path in glob(pattern):
                idl = IDLParser().parse(path)
                self.msgs += idl["msgs"]
                self.consts += idl["consts"]
                self.typedefs += idl["typedefs"]
                self.enums += idl["enums"]
        elif tree.children[0].data == "type_stmt":
            typedef = self.visit_type_stmt(tree.children[0])
            typedef["namespace"] = namespace
            self.typedefs.append(typedef)
        elif tree.children[0].data == "const_stmt":
            const = self.visit_const_stmt(tree.children[0])
            const["namespace"] = namespace
            self.consts.append(const)
        elif tree.children[0].data == "enum_stmt":
            enum = self.visit_enum_stmt(tree.children[0])
            enum["namespace"] = namespace
            self.enums.append(enum)
        else:
            # unreachable
            assert False

    def visit_type_stmt(self, tree):
        assert tree.data == "type_stmt"
        return {
            "name": tree.children[0].value,
            "type": self.visit_type(tree.children[1]),
        }

    def visit_const_stmt(self, tree):
        assert tree.data == "const_stmt"
        return {
            "name": tree.children[0].value,
            "type": self.visit_type(tree.children[1]),
            "value": tree.children[2].value,
        }

    def visit_enum_stmt(self, tree):
        assert tree.data == "enum_stmt"
        return {
            "name": tree.children[0].value,
            "enum_items": list(map(self.visit_enum_item, tree.children[1].children)),
        }

    def visit_enum_item(self, tree):
        assert tree.data == "enum_item"
        return {
            "name": tree.children[0].value,
            "value": tree.children[1].value,
        }

    def is_ool_field_type(self, typename):
        ool_builtins = ["str", "bytes"]
        return typename in ool_builtins

    def visit_msg_def(self, tree):
        global next_msg_id
        assert tree.data == "msg_def"
        modifiers = list(map(lambda x: x.value, tree.children[0].children))
        name = tree.children[1].value
        args = self.visit_fields(tree.children[2].children)
        args_id = next_msg_id
        next_msg_id += 1

        is_oneway = "oneway" in modifiers or "async" in modifiers
        if len(tree.children) > 3:
            rets = self.visit_fields(tree.children[3].children)
            rets_id = next_msg_id
            next_msg_id += 1
        else:
            if not is_oneway:
                raise Exception(f"{name}: return values is not specified")
            rets = []
            rets_id = None

        msg_def = {
            "args_id": args_id,
            "rets_id": rets_id,
            "name": name,
            "oneway": is_oneway,
            "args": args,
            "rets": rets,
        }

        return msg_def

    def visit_fields(self, trees):
        ool_field = None
        inline_fields = []
        for tree in trees:
            field = self.visit_field(tree)
            if self.is_ool_field_type(field["type"]["name"]):
                if ool_field:
                    raise Exception(f"{name}: multiple ool fields are not allowed: {ool_field['name']}, {field['name']}")
                ool_field = field
                ool_field["is_str"] = field["type"]["name"] == "str"
            else:
                inline_fields.append(field)

        return {
            "inlines": inline_fields,
            "ool": ool_field,
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
    user_types = {}
    builtins = dict(
        str="const char *",
        bytes="const void *",
        char="char",
        int="int",
        task="task_t",
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
        if fields["ool"]:
            flags += "| MSG_OOL"
            if fields["ool"]["is_str"]:
                flags += "| MSG_STR"
        return flags

    def resolve_type(ns, type_):
        prefix = f"{ns}_".lstrip("_")
        if type_["name"] in user_types:
            if user_types[type_["name"]] == "enum":
                return f"enum {prefix}{type_['name']}"
            elif user_types[type_["name"]] == "typedef":
                return f"{prefix}{type_['name']}_t"
            else:
                assert False # unreachable
        else:
            return builtins.get(type_["name"], f"{type_['name']}_t")

    def field_def(field, ns):
        type_ = field["type"]
        def_ = resolve_type(ns, type_)
        def_ += f" {field['name']}"
        if type_["nr"]:
            def_ += f"[{type_['nr']}]"
        return def_

    def const_def(c):
        type_ = c["type"]
        assert type_["nr"] is None
        name = (f"{c['namespace']}_".lstrip("_") + c['name']).upper()
        return f"static const {resolve_type(c['namespace'], type_)} {name} = {c['value']}"

    def type_def(t):
        type_ = t["type"]
        prefix = f"{t['namespace']}_".lstrip("_")
        def_ = f"typedef {resolve_type(t['namespace'], type_)} {prefix}{t['name']}_t"
        if type_["nr"]:
            def_ += f"[{type_['nr']}]"
        return def_

    for type_ in idl["enums"]:
        user_types[type_["name"]] = "enum"
    for type_ in idl["typedefs"]:
        user_types[type_["name"]] = "typedef"

    renderer = jinja2.Environment()
    renderer.filters["msg_name"] = lambda m: f"{m['namespace']}_".lstrip("_") + m['name']
    renderer.filters["new_type_name"] = lambda t: f"{t['namespace']}_".lstrip("_") + t['name']
    renderer.filters["enum_name"] = lambda t: f"{t['namespace']}_".lstrip("_") + t['name']
    renderer.filters["enum_item_name"] = lambda i,e: (f"{e['namespace']}_".lstrip("_") + f"{e['name']}_".lstrip("_") + i['name']).upper()
    renderer.filters["field_def"] = field_def
    renderer.filters["const_def"] = const_def
    renderer.filters["type_def"] = type_def
    renderer.filters["msg_type"] = msg_type
    renderer.filters["msg_str"] = lambda m: f"{m['namespace']}.".lstrip(".") + m['name']
    template = renderer.from_string ("""\
#ifndef __GENIDL_MESSAGE_H__
#define __GENIDL_MESSAGE_H__
//
//  Generated by genidl.py. DO NOT EDIT!
//
#include <types.h>

//
//  Constants
//
{% for c in consts %}
{{ c | const_def }};
{% endfor %}

//
//  typedefs
//
{% for t in typedefs %}
{{ t | type_def }};
{% endfor %}

//
//  Enums
//
{% for e in enums %}
enum {{ e | enum_name }} {{ "{" }}
    {%- for item in e.enum_items %}
    {{ item | enum_item_name(e) }} = {{ item.value }},
    {%- endfor %}
{{ "};" }}
{% endfor %}

//
//  Message Fields
//
{% for msg in msgs %}
struct {{ msg | msg_name }}_fields {{ "{" }}
{%- if msg.args.ool %}
    {%- if msg.args.ool.is_str %}
    const char *{{ msg.args.ool.name }};
    {% else %}
    const void *{{ msg.args.ool.name }};
    {% endif %}
    size_t {{ msg.args.ool.name }}_len;
{% endif %}
{%- for field in msg.args.inlines %}
    {{ field | field_def(msg.namespace) }};
{%- endfor %}
{{ "};" }}

{%- if not msg.oneway %}
struct {{ msg | msg_name }}_reply_fields {{ "{" }}
{%- if msg.rets.ool %}
    {%- if msg.rets.ool.is_str %}
    const char *{{ msg.rets.ool.name }};
    {% else %}
    const void *{{ msg.rets.ool.name }};
    {% endif %}
    size_t {{ msg.rets.ool.name }}_len;
{% endif %}
{%- for field in msg.rets.inlines %}
    {{ field | field_def(msg.namespace) }};
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
{%- if msg.args.ool %}
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}.{{ msg.args.ool.name }}) == offsetof(struct message, ool_ptr)); \\
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}.{{ msg.args.ool.name }}_len) == offsetof(struct message, ool_len)); \\
{%- endif %}
{%- if not msg.oneway %}
{%- if msg.rets.ool %}
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}_reply.{{ msg.rets.ool.name }}) == offsetof(struct message, ool_ptr)); \\
    STATIC_ASSERT(offsetof(struct message, {{ msg | msg_name }}_reply.{{ msg.rets.ool.name }}_len) == offsetof(struct message, ool_len)); \\
{%- endif %}
{%- endif %}
{%- endfor %}

#define IDL_MSGID_MAX {{ msgid_max }}
#define IDL_MSGID2STR \\
    (const char *[]){{ "{" }} \\
    {% for m in msgs %} \\
        [{{ m.args_id }}] = "{{ m | msg_str }}", \\
        {%- if not m.oneway %}
        [{{ m.rets_id }}] = "{{ m | msg_str }}_reply", \\
        {%- endif %}
    {% endfor %} \\
    {{ "}" }}

#endif

""")

    msgid_max = next_msg_id - 1
    text = template.render(msgid_max=msgid_max,**idl)

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
