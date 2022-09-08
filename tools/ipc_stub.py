#!/usr/bin/env python3
import sys
import argparse
import jinja2
import markdown
from pathlib import Path
from glob import glob
from lark import Lark
from lark.exceptions import LarkError
from colorama import Fore, Style

parser = Lark(
    r"""
%import common.WS
%ignore WS
%ignore COMMENT
%ignore EMPTY_COMMENT
COMMENT: /\/\/[^\/][^\n]*/
EMPTY_COMMENT: /\/\/[^\/]/
NAME: /[a-zA-Z_][a-zA-Z_0-9]*/
PATH: /"[^"]+"/
NATINT: /[1-9][0-9]*/
CONST: /(0x[0-9]+)|([1-9][0-9]*)|0/

start: stmts?
stmts: stmt*
stmt:
    | doc_comment
    | msg_def
    | namespace_def
    | include_stmt
    | enum_stmt
    | type_stmt
    | const_stmt

doc_comment: "///" /[^\n]*\n/
type_stmt: "type" NAME "=" type ";"
const_stmt: "const" NAME ":" type "=" CONST ";"
enum_stmt: "enum" NAME "{" enum_items "}" ";"
enum_items: (enum_item ("," enum_item)*)? ","?
enum_item: NAME "=" CONST
include_stmt: "include" PATH ";"
namespace_def: "namespace" NAME "{" stmts "}"
msg_def: modifiers? MSG_TYPE NAME "(" fields ")" ["->" "(" fields ")"] ";"
modifiers: MODIFIER*
MSG_TYPE: "rpc" | "oneway"
MODIFIER: "async"
fields: (field ("," field)*)?
field: NAME ":" type
type: NAME ("[" NATINT "]")?
"""
)


class ParseError(Exception):
    def __init__(self, message, hint=None):
        self.message = message
        self.hint = hint


next_msg_id = 1


class IDLParser:
    def __init__(self):
        self.msgs = []
        self.typedefs = []
        self.consts = []
        self.enums = []
        self.namespaces = {
            "": dict(name="global", doc="", messages=[], types=[], consts=[])
        }
        self.message_context = None
        self.namespace_contexts = [None]
        self.doc_comment = ""
        self.prev_stmt = ""

    def parse(self, text):
        ast = parser.parse(text)
        for stmt in ast.children[0].children:
            self.visit_stmt("", stmt)

        return {
            "namespaces": self.namespaces,
            "msgs": self.msgs,
            "consts": self.consts,
            "typedefs": self.typedefs,
            "enums": self.enums,
        }

    def current_ctx(self):
        ns_name = self.namespace_contexts[-1]
        if ns_name is None:
            ctx = "(root)"
        else:
            ctx = ns_name

        if self.message_context is not None:
            ctx += f".{self.message_context}"

        return ctx

    def get_doc_comment(self):
        doc = self.doc_comment
        self.doc_comment = ""
        return doc

    def visit_stmt(self, namespace, tree):
        assert tree.data == "stmt"
        stmt = tree.children[0]
        if stmt.data == "namespace_def":
            name = stmt.children[0].value
            self.namespaces[name] = dict(
                name=name, doc=self.get_doc_comment(), messages=[], types=[], consts=[]
            )
            self.namespace_contexts.append(name)
            for stmt in stmt.children[1].children:
                self.visit_stmt(name, stmt)
            self.namespace_contexts.pop()
        elif stmt.data == "msg_def":
            msg_def = self.visit_msg_def(stmt)
            msg_def["doc"] = self.get_doc_comment()
            msg_def["namespace"] = namespace
            self.msgs.append(msg_def)
            self.namespaces[namespace]["messages"].append(msg_def)
        elif stmt.data == "include_stmt":
            pattern = stmt.children[0].value.strip('"')
            for path in glob(pattern):
                idl = IDLParser().parse(path)
                self.msgs += idl["msgs"]
                self.consts += idl["consts"]
                self.typedefs += idl["typedefs"]
                self.enums += idl["enums"]
        elif stmt.data == "type_stmt":
            typedef = self.visit_type_stmt(stmt)
            typedef["namespace"] = namespace
            typedef["doc"] = self.get_doc_comment()
            self.typedefs.append(typedef)
            self.namespaces[namespace]["types"].append(typedef)
        elif stmt.data == "const_stmt":
            const = self.visit_const_stmt(stmt)
            const["namespace"] = namespace
            const["doc"] = self.get_doc_comment()
            self.consts.append(const)
            self.namespaces[namespace]["consts"].append(const)
        elif stmt.data == "enum_stmt":
            enum = self.visit_enum_stmt(stmt)
            enum["namespace"] = namespace
            self.enums.append(enum)
        elif stmt.data == "doc_comment":
            line = str(stmt.children[0])
            if line.startswith(" "):
                line = line[1:]
            if self.prev_stmt and self.prev_stmt.data == "doc_comment":
                self.doc_comment += "\n" + line
            else:
                self.doc_comment = line
            self.doc_comment = self.doc_comment.strip()
        else:
            # unreachable
            assert False
        self.prev_stmt = stmt

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

    def visit_msg_def(self, tree):
        global next_msg_id
        assert tree.data == "msg_def"
        name = tree.children[2].value
        self.message_context = name

        modifiers = list(map(lambda x: x.value, tree.children[0].children))
        msg_type = tree.children[1].value
        name = tree.children[2].value
        args = self.visit_fields(tree.children[3].children)
        id = next_msg_id
        next_msg_id += 1

        is_oneway = "oneway" == msg_type or "async" in modifiers
        if len(tree.children) > 4:
            rets = self.visit_fields(tree.children[4].children)
            reply_id = next_msg_id
            next_msg_id += 1
        else:
            if not is_oneway:
                raise ParseError(
                    f"{self.current_ctx()}: return values is not specified",
                    "Add '-> ()' or consider defining it as 'oneway' message",
                )
            rets = []
            reply_id = None

        msg_def = {
            "id": id,
            "reply_id": reply_id,
            "name": name,
            "oneway": is_oneway,
            "modifiers": modifiers,
            "msg_type": msg_type,
            "args": args,
            "rets": rets,
        }

        self.message_context = None
        return msg_def

    def visit_fields(self, trees):
        fields = []
        for tree in trees:
            field = self.visit_field(tree)
            fields.append(field)

        return {
            "fields": fields,
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
        char="char",
        bool="bool",
        int="int",
        uint="unsigned",
        int8="int8_t",
        int16="int16_t",
        int32="int32_t",
        int64="int64_t",
        uint8="uint8_t",
        uint16="uint16_t",
        uint32="uint32_t",
        uint64="uint64_t",
        size="size_t",
        offset="offset_t",
        uaddr="uaddr_t",
        paddr="paddr_t",
        task="task_t",
    )

    def resolve_type(ns, type_):
        prefix = f"{ns}_".lstrip("_")
        if type_["name"] in user_types:
            if user_types[type_["name"]] == "enum":
                return f"enum {prefix}{type_['name']}"
            elif user_types[type_["name"]] == "typedef":
                return f"{prefix}{type_['name']}_t"
            else:
                assert False  # unreachable
        else:
            resolved_type = builtins.get(type_["name"])
            if resolved_type is None:
                raise ParseError(f"Uknown data type: '{type_['name']}'")
            return resolved_type

    def field_defs(fields, ns):
        defs = []
        for field in fields:
            type_ = field["type"]
            if type_["name"] == "bytes":
                defs.append(f"uint8_t {field['name']}[{type_['nr']}]")
                defs.append(f"size_t {field['name']}_len")
            elif type_["name"] == "cstr":
                defs.append(f"char {field['name']}[{type_['nr']}]")
            else:
                def_ = resolve_type(ns, type_)
                def_ += f" {field['name']}"
                if type_["nr"]:
                    def_ += f"[{type_['nr']}]"
                defs.append(def_)
        return defs

    def const_def(c):
        type_ = c["type"]
        assert type_["nr"] is None
        name = (f"{c['namespace']}_".lstrip("_") + c["name"]).upper()
        return (
            f"static const {resolve_type(c['namespace'], type_)} {name} = {c['value']}"
        )

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
    renderer.filters["msg_name"] = (
        lambda m: f"{m['namespace']}_".lstrip("_") + m["name"]
    )
    renderer.filters["new_type_name"] = (
        lambda t: f"{t['namespace']}_".lstrip("_") + t["name"]
    )
    renderer.filters["enum_name"] = (
        lambda t: f"{t['namespace']}_".lstrip("_") + t["name"]
    )
    renderer.filters["enum_item_name"] = lambda i, e: (
        f"{e['namespace']}_".lstrip("_") + f"{e['name']}_".lstrip("_") + i["name"]
    ).upper()
    renderer.filters["newlines_to_whitespaces"] = lambda text: text.replace("\n", " ")
    renderer.filters["field_defs"] = field_defs
    renderer.filters["const_def"] = const_def
    renderer.filters["type_def"] = type_def
    renderer.filters["msg_str"] = lambda m: f"{m['namespace']}.".lstrip(".") + m["name"]
    template = renderer.from_string(
        """\
#pragma once
//
//  Generated by ipcstub.py. DO NOT EDIT!
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
{%- for field in msg.args.fields | field_defs(msg.namespace) %}
    {{ field }};
{%- endfor %}
{{ "};" }}

{%- if not msg.oneway %}
struct {{ msg | msg_name }}_reply_fields {{ "{" }}
{%- for field in msg.rets.fields | field_defs(msg.namespace) %}
    {{ field }};
{%- endfor %}
{{ "};" }}
{%- endif %}
{% endfor %}

#define __DEFINE_MSG_TYPE(type, len) ((type) << 12 | (len))

{% for msg in msgs %}
#define {{ msg | msg_name | upper }}_MSG __DEFINE_MSG_TYPE({{ msg.id }}, sizeof(struct {{ msg | msg_name }}_fields))
{%- if not msg.oneway %}
#define {{ msg | msg_name | upper }}_REPLY_MSG __DEFINE_MSG_TYPE({{ msg.reply_id }}, sizeof(struct {{ msg | msg_name }}_fields))
{%- endif %}
{%- endfor %}

//
//  Macros
//
#define IPCSTUB_MESSAGE_FIELDS \\
{%- for msg in msgs %}
    struct {{ msg | msg_name }}_fields {{ msg | msg_name }}; \\
{%- if not msg.oneway %}
    struct {{ msg | msg_name }}_reply_fields {{ msg | msg_name }}_reply; \\
{%- endif %}
{%- endfor %}

#define IPCSTUB_MSGID_MAX {{ msgid_max }}
#define IPCSTUB_MSGID2STR \\
    (const char *[]){{ "{" }} \\
    {% for m in msgs %} \\
        [{{ m.id }}] = "{{ m | msg_str }}", \\
        {%- if not m.oneway %}
        [{{ m.reply_id }}] = "{{ m | msg_str }}_reply", \\
        {%- endif %}
    {% endfor %} \\
    {{ "}" }}

#define IPCSTUB_STATIC_ASSERTIONS \\
{%- for msg in msgs %}
    _Static_assert( \\
        sizeof(struct {{ msg | msg_name }}_fields) < 4096, \\
        "'{{ msg | msg_name }}' message is too large, should be less than 4096 bytes" \\
    ); \\
{%- if not msg.oneway %}
    _Static_assert( \\
        sizeof(struct {{ msg | msg_name }}_reply_fields) < 4096, \\
        "'{{ msg | msg_name }}_reply' message is too large, should be less than 4096 bytes" \\
    ); \\
{%- endif %}
{%- endfor %}


"""
    )

    msgid_max = next_msg_id - 1
    text = template.render(msgid_max=msgid_max, **idl)

    with open(args.out, "w") as f:
        f.write(text)


def main():
    parser = argparse.ArgumentParser(description="The IPC stubs generator.")
    parser.add_argument("idls", nargs="+", help="The IDL file.")
    parser.add_argument(
        "--lang",
        choices=["c"],
        default="c",
        help="The output language.",
    )
    parser.add_argument(
        "-o",
        dest="out",
        required=True,
        help="The output file (for c/html) or directory (for rust).",
    )
    args = parser.parse_args()

    concated_idl = ""
    for file in args.idls:
        with open(file, "r") as f:
            concated_idl += f.read()

    try:
        idl = IDLParser().parse(concated_idl)
        if args.lang == "c":
            c_generator(args, idl)
    except ParseError as e:
        print(
            f"ipc_stub: {Fore.RED}{Style.BRIGHT}error:{Fore.RESET} {e.message}{Style.RESET_ALL}"
        )
        if e.hint is not None:
            print(
                f"{Fore.YELLOW}{Style.BRIGHT}    | Hint:{Fore.RESET} {e.hint}{Style.RESET_ALL}"
            )
        sys.exit(1)
    except LarkError as e:
        print(
            f"ipc_stub: {Style.BRIGHT}{Fore.RED}error:{Fore.RESET} {e}{Style.RESET_ALL}"
        )


if __name__ == "__main__":
    main()
