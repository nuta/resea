#!/usr/bin/env python3
import sys
import argparse
import jinja2
import markdown
from glob import glob
from lark import Lark
from lark.exceptions import LarkError
from colorama import Fore, Back, Style

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
        self.namespaces = { "": dict(name="global", doc="", messages=[], types=[], consts=[]) }
        self.message_context = None
        self.namespace_contexts = [None]
        self.doc_comment = ""
        self.prev_stmt = ""

    def parse(self, path):
        parser = Lark(r"""
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
        """)

        ast = parser.parse(open(path).read())
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
                name=name, doc=self.get_doc_comment(), messages=[], types=[], consts=[])
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

    def is_ool_field_type(self, typename):
        ool_builtins = ["str", "bytes"]
        return typename in ool_builtins

    def visit_msg_def(self, tree):
        global next_msg_id
        assert tree.data == "msg_def"
        name = tree.children[2].value
        self.message_context = name

        modifiers = list(map(lambda x: x.value, tree.children[0].children))
        msg_type = tree.children[1].value
        name = tree.children[2].value
        args = self.visit_fields(tree.children[3].children)
        args_id = next_msg_id
        next_msg_id += 1

        is_oneway = "oneway" == msg_type or "async" in modifiers
        if len(tree.children) > 4:
            rets = self.visit_fields(tree.children[4].children)
            rets_id = next_msg_id
            next_msg_id += 1
        else:
            if not is_oneway:
                raise ParseError(
                    f"{self.current_ctx()}: return values is not specified",
                    "Add '-> ()' or consider defining it as 'oneway' message"
                )
            rets = []
            rets_id = None

        msg_def = {
            "args_id": args_id,
            "rets_id": rets_id,
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
        ool_field = None
        inline_fields = []
        fields = []
        for tree in trees:
            field = self.visit_field(tree)
            if self.is_ool_field_type(field["type"]["name"]):
                if ool_field:
                    raise ParseError(
                        f"{self.current_ctx()}: Multiple ool fields (bytes or str) are not allowed: "
                            f"{ool_field['name']}, {field['name']}",
                    )
                ool_field = field
                ool_field["is_str"] = field["type"]["name"] == "str"
            else:
                inline_fields.append(field)
            fields.append(field)

        return {
            "fields": fields,
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
        str="char *",
        bytes="void *",
        char="char",
        bool="bool",
        int="int",
        task="task_t",
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
        notifications="notifications_t",
        vaddr="vaddr_t",
        paddr="paddr_t",
        trap_frame="trap_frame_t",
        handle="handle_t",
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
            resolved_type = builtins.get(type_["name"])
            if resolved_type is None:
                raise ParseError(
                    f"Uknown data type: '{type_['name']}'"
                )
            return resolved_type

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
    renderer.filters["newlines_to_whitespaces"] = lambda text: text.replace("\n", " ")
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
    char *{{ msg.args.ool.name }};
    {% else %}
    void *{{ msg.args.ool.name }};
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
    char *{{ msg.rets.ool.name }};
    {% else %}
    void *{{ msg.rets.ool.name }};
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
{%- for msg in msgs %} \\
    /// {{  msg.doc | newlines_to_whitespaces }}  \\
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

def html_generator(args, idl):
    renderer = jinja2.Environment()
    renderer.filters["md2html"] = lambda text: markdown.markdown(text)
    renderer.filters["params"] = \
        lambda params: ", ".join(map(lambda p: f"{p['name']}: {p['type']['name']}", params["fields"]))
    css = """\
body {
    font-family: sans-serif;
    color: #333;
    line-height: 1.8rem;
}

.wallpaper {
    max-width: 900px;
    margin: 20px auto 0;
}

h2 {
    color: #eee;
    background: #507ab0;
    padding: 10px 15px 10px;
    margin-top: 2rem;
}

h3 {
    border-top: 1px solid #afafaf;
    padding-top: 20px;
}

h3 > .ident {
    color: #119647;
}

code {
    font-family: Menlo, monospace;
    padding: 2px 4px;
    background: #f2f2f2;
    font-size: 0.95rem;
}

.codeblock {
    font-family: Menlo, monospace;
    padding: 1.2rem 1rem;
    background: #f2f2f2;
    font-size: 0.95rem;
}

.doc {
    margin-top: 1.5rem;
    margin-bottom: 3rem;
}
"""
    template = renderer.from_string ("""\
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Resea Message Interfaces</title>
    <style>{{ css }}</style>
</head>
<body>
    <div class="wallpaper">
        <nav>
            <h1>Resea Message Interfaces</h1>
            <p>Resea message interface definitions generated by genidl.py.</p>
        </nav>
        <main>
            {% for ns in namespaces.values() %}
            <div>
                <h2>{{ ns.name }} interface</h2>
                <div class="doc">
                    {{ ns.doc | md2html }}
                </div>
                <section>
                    {% for m in ns.messages %}
                        <h3>
                            {{ m.modifiers | join(" ") }}
                            {% if m.oneway %} message {% else %} rpc {% endif %}
                            <span class="ident">
                                {{ m.name }}
                            </span>
                        </h3>
                        <div class="codeblock">
                        {% if m.oneway %}
                            oneway {{ m.name }}({{ m.args | params }})
                        {% else %}
                            rpc {{ m.name }}({{ m.args | params }}) -> ({{ m.rets | params }})
                        {% endif %}
                        </div>
                        <div class="doc">
                            {{ m.doc | md2html }}
                        </div>
                    {% endfor %}
                </section>
            </div>
            {% endfor %}
        </main>
    </div>
</body>
</html>
""")
    text = template.render(css=css, **idl)
    with open(args.out, "w") as f:
        f.write(text)

def main():
    parser = argparse.ArgumentParser(
        description="The message definitions generator.")
    parser.add_argument("--idl", required=True, help="The IDL file.")
    parser.add_argument("--lang", choices=["c", "html"], default="c",
        help="The output language.")
    parser.add_argument("-o", dest="out", required=True,
        help="The output directory.")
    args = parser.parse_args()

    try:
        idl = IDLParser().parse(args.idl)
        if args.lang == "c":
            c_generator(args, idl)
        elif args.lang == "html":
            html_generator(args, idl)
    except ParseError as e:
        print(f"genidl: {Fore.RED}{Style.BRIGHT}error:{Fore.RESET} {e.message}{Style.RESET_ALL}")
        if e.hint is not None:
            print(f"{Fore.YELLOW}{Style.BRIGHT}    | Hint:{Fore.RESET} {e.hint}{Style.RESET_ALL}")
        sys.exit(1)
    except LarkError as e:
        print(f"genidl: {Style.BRIGHT}{Fore.RED}error:{Fore.RESET} {e}{Style.RESET_ALL}")

if __name__ == "__main__":
    main()
