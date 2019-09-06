#!/usr/bin/env python3
import argparse
import jinja2
import re
from lark import Lark, Transformer

GRAMMAR = """
ID: /[a-zA-Z_][a-zA-Z0-9_]*/
VALUE: /[a-zA-Z0-9_-][a-zA-Z0-9_-]*/
COMMENT: /\/\/.*/
%import common.WS
%ignore WS
%ignore COMMENT
idl: interfaces
interfaces: interface*
interface: attrs "interface" ID "{" types messages "}"
attrs: "[" [attr ("," attr)*] "]"
attr: ID "(" VALUE ")"
types: type*
type: [attrs] "type" ID "=" VALUE
messages: message*
message: [attrs] ID args "->" args
args: "(" [arg ("," arg)*] ")"
arg: ID ":" ID
"""

class InvalidIDLError(Exception):
    pass

class IdlTransformer(Transformer):
    tokens_to_tuple = lambda _, ts: tuple(map(str, ts))
    interfaces = list
    types = list
    messages = list
    attrs = dict
    attr = tokens_to_tuple
    args = list
    arg = tokens_to_tuple
    defs = list
    idl = lambda _, child: child[0]
    def message(self, child):
        attrs, name, args, rets = child

        if "id" not in attrs:
            raise InvalidIDLError("The interface ID must be specified.")

        def visit_params(fields):
            pages = []
            inlines = []
            new_fields = []
            for field in fields:
                new_fields.append({ "name": field[0], "type": field[1] })
                if field[1] == "page":
                    pages.append({ "name": field[0], "type": field[1] })
                else:
                    inlines.append({ "name": field[0], "type": field[1] })
            return { "pages": pages, "inlines": inlines, "fields": new_fields }

        return {
            "attrs": attrs,
            "name": str(name),
            "args": visit_params(args),
            "rets": visit_params(rets),
        }

    def type(self, child):
        name, alias_of = child
        return {
            "name": str(name),
            "alias_of": str(alias_of),
        }

    def interface(self, child):
        attrs, name, types, messages = child
        if "id" not in attrs:
            raise InvalidIDLError("The interface ID must be specified.")
        return {
            "name": str(name),
            "attrs": attrs,
            "types": types,
            "messages": messages,
        }

TEMPLATE = """\
#ifndef __RESEA_IDL_H__
#define __RESEA_IDL_H__
#ifdef KERNEL
#include <ipc.h>
typedef vaddr_t uintptr_t;
#else
#include <resea.h>
#endif

#pragma clang diagnostic push
// FIXME: Support padding in the message structs.
#pragma clang diagnostic ignored "-Waddress-of-packed-member"

#define MSG_REPLY_FLAG (1ULL << 7)

{%- for interface in interfaces %}
//
//  {{ interface.name }}
//
#define {{ interface.name | upper }}_INTERFACE  {{ interface.attrs.id }}ULL
{%- for type in interface.types %}
typedef {{ type.alias_of }}_t {{ type.name }}_t;
{%- endfor %}

{%- for msg in interface.messages %}
#define {{ msg.name | upper }}_MSG (({{ interface.name | upper }}_INTERFACE << 8) | {{ msg.attrs.id }}ULL)
#define {{ msg.name | upper }}_INLINE_LEN ({{ msg.args | inline_len }})
#define {{ msg.name | upper }}_HEADER \
    ( ({{ msg.name | upper }}_MSG        << MSG_TYPE_OFFSET) \
    | ({{ msg.args.pages | length }}     << MSG_NUM_PAGES_OFFSET) \
    | ({{ msg.name | upper }}_INLINE_LEN << MSG_INLINE_LEN_OFFSET) \
    )

{%- if msg.attrs.type == "call" %}
#define {{ msg.name | upper }}_REPLY_MSG (({{ interface.name | upper }}_INTERFACE << 8) | MSG_REPLY_FLAG | {{ msg.attrs.id }})
#define {{ msg.name | upper }}_REPLY_INLINE_LEN ({{ msg.rets | inline_len }})
#define {{ msg.name | upper }}_REPLY_HEADER \
    ( ({{ msg.name | upper }}_REPLY_MSG        << MSG_TYPE_OFFSET) \
    | ({{ msg.rets.pages | length }}           << MSG_NUM_PAGES_OFFSET) \
    | ({{ msg.name | upper }}_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET) \
    )

struct {{ msg.name }}_msg {
    uint32_t header;
    cid_t from;
    cid_t channels[4];
{%- for field in msg.args.pages %}
    page_t {{ field.name }};
{%- endfor %}
    page_t __unused_pages[4 - {{ msg.args.pages | length }}];
{%- for field in msg.args.inlines %}
    {{ field.type | rename_type }} {{ field.name }};
{%- endfor %}
    uint8_t __unused[INLINE_PAYLOAD_LEN_MAX - {{ msg.name | upper }}_INLINE_LEN];
};

struct {{ msg.name }}_reply_msg {
    uint32_t header;
    cid_t from;
    cid_t __unused_channels[4];
{%- for field in msg.rets.pages %}
    page_t {{ field.name }};
{%- endfor %}
    page_t __unused_pages[4 - {{ msg.rets.pages | length }}];
{%- for field in msg.rets.inlines %}
    {{ field.type | rename_type }} {{ field.name }};
{%- endfor %}
    uint8_t __unused[INLINE_PAYLOAD_LEN_MAX - {{ msg.name | upper }}_INLINE_LEN];
} PACKED;

#if !defined(KERNEL)
static inline error_t {{ msg.name }}({{ msg | call_params }}) {
    struct {{ msg.name }}_msg m;
    struct {{ msg.name }}_reply_msg r;

    m.header = {{ msg.name | upper }}_HEADER;
{% for arg in msg.args.fields %}
    m.{{ arg.name }} = {{ arg.name }};
{%- endfor %}
    error_t err;
    if ((err = ipc_call(__ch, (struct message *) &m, (struct message *) &r)) != OK)
        return err;
{%- for ret in msg.rets.fields %}
    *{{ ret.name }} = r.{{ ret.name }};
{%- endfor %}
    return OK;
}

typedef error_t (*{{ msg.name }}_handler_t)(struct {{ msg.name }}_msg *m, struct {{ msg.name }}_reply_msg *r);

static inline header_t dispatch_{{ msg.name }}({{ msg.name }}_handler_t handler, struct message *m, struct message *r) {
    if (m->header != {{ msg.name | upper }}_HEADER) {
        return ERR_INVALID_HEADER;
    }

    error_t err = handler((struct {{ msg.name }}_msg *) m, (struct {{ msg.name }}_reply_msg *) r);
    if (err != ERR_DONT_REPLY) {
        r->header = (err == OK) ? {{ msg.name | upper }}_REPLY_HEADER : ERROR_TO_HEADER(err);
    }
    return err;
}
#endif

{%- endif %}
{%- endfor %}
{%- endfor %}

#pragma clang diagnostic pop

#endif
"""

builtin_types = {
    "int8":     { "size": "sizeof(int8_t)" },
    "int16":    { "size": "sizeof(int16_t)" },
    "int32":    { "size": "sizeof(int32_t)" },
    "int64":    { "size": "sizeof(int64_t)" },
    "uint8":    { "size": "sizeof(uint8_t)" },
    "uint16":   { "size": "sizeof(uint16_t)" },
    "uint32":   { "size": "sizeof(uint32_t)" },
    "uint64":   { "size": "sizeof(uint64_t)" },
    "pid":      { "size": "sizeof(pid_t)" },
    "tid":      { "size": "sizeof(tid_t)" },
    "cid":      { "size": "sizeof(cid_t)" },
    "uintptr":  { "size": "sizeof(uintptr_t)" },
    "size":     { "size": "sizeof(size_t)" },
    "page":     { "size": "sizeof(page_t)" },
}

user_defined_types = {}

# Resolves a type name to a corresponding builtin type.
def resolve_type(type_name):
    if type_name in builtin_types.keys():
        return type_name
    if type_name not in user_defined_types:
        raise InvalidIDLError(f"Invalid type name: '{type_name}'")
    return user_defined_types[type_name]

# Returns the type name in C.
def rename_type(type_name):
    return resolve_type(type_name) + "_t"

def inline_len(params):
    sizes = []
    for field in params["inlines"]:
        type_ = builtin_types[resolve_type(field["type"])]
        sizes.append(type_["size"])
    if len(sizes) == 0:
        return "0"
    else:
        return " + ".join(sizes)

def call_params(m):
    params = ["cid_t __ch"]
    for arg in m["args"]["fields"]:
        params.append(f"{rename_type(arg['type'])} {arg['name']}")
    for ret in m["rets"]["fields"]:
        params.append(f"{rename_type(ret['type'])} *{ret['name']}")
    return ", ".join(params)

def genstub(interfaces):
    global types
    for interface in interfaces:
        user_defined_types.update(interface["types"])

    renderer = jinja2.Environment()
    renderer.filters["rename_type"] = rename_type
    renderer.filters["inline_len"] = inline_len
    renderer.filters["call_params"] = call_params
    print(renderer.from_string(TEMPLATE).render(interfaces=interfaces))

def main():
    parser = argparse.ArgumentParser(description="The C stub generator.")
    parser.add_argument("idl_file", help="The IDL file.")
    args = parser.parse_args()

    ast = Lark(GRAMMAR, start="idl").parse(open(args.idl_file).read())
    idl = IdlTransformer().transform(ast)
    genstub(idl)

if __name__ == "__main__":
    main()
