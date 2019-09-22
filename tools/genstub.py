#!/usr/bin/env python3
import argparse
from pathlib import Path
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
            page = None
            channel = None
            inlines = []
            new_fields = []
            for field in fields:
                new_fields.append({ "name": field[0], "type": field[1] })
                if field[1] == "page":
                    if page is not None:
                        raise InvalidIDLError("Multiple page payloads in single"
                        "message is not supported")
                    page = { "name": field[0], "type": "page" }
                elif field[1] == "channel":
                    if page is not None:
                        raise InvalidIDLError("Multiple channel payloads in "
                        "single message is not supported")
                    channel = { "name": field[0], "type": "channel" }
                else:
                    inlines.append({ "name": field[0], "type": field[1] })
            return {
                "page": page,
                "channel": channel,
                "inlines": inlines,
                "fields": new_fields,
            }

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
        for message in messages:
            message["canon_name"] = f"{name}_{message['name']}"
        return {
            "name": str(name),
            "attrs": attrs,
            "types": types,
            "messages": messages,
        }

CLIENT_STUBS = """
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
#define {{ msg.canon_name | upper }}_MSG (({{ interface.name | upper }}_INTERFACE << 8) | {{ msg.attrs.id }}ULL)
#define {{ msg.canon_name | upper }}_INLINE_LEN ({{ msg.args | inline_len }})
#define {{ msg.canon_name | upper }}_HEADER \
    ( ({{ msg.canon_name | upper }}_MSG        << MSG_TYPE_OFFSET) \
{%- if msg.args.page -%}
    | MSG_PAGE_PAYLOAD  \
{%- endif -%}
{%- if msg.args.channel -%}
    | MSG_CHANNEL_PAYLOAD  \
{%- endif -%}
    | ({{ msg.canon_name | upper }}_INLINE_LEN << MSG_INLINE_LEN_OFFSET) \
    )

#if !defined(KERNEL)
static inline error_t __do_send_{{ msg.canon_name }}({{ msg.args | send_params }}, bool __async) {
    struct message m;
    m.header = {{ msg.canon_name | upper }}_HEADER;
{% for field in msg.args.fields %}
{%- if field.type == "smallstring" %}
    strcpy_s((char *) &m.payloads.{{ interface.name }}.{{ msg.name }}.{{ field.name }}, SMALLSTRING_LEN_MAX, {{ field.name }});
{%- else %}
    m.payloads.{{ interface.name }}.{{ msg.name }}.{{ field.name }} = {{ field.name }};
{%- endif %}
{%- endfor %}
    error_t err = __async ? ipc_async_send(__ch, &m) : ipc_send(__ch, &m);
    if (err != OK) {
        return err;
    }
    return OK;
}

static inline error_t send_{{ msg.canon_name }}({{ msg.args | send_params }}) {
    return __do_send_{{ msg.canon_name }}(__ch
{%- for field in msg.args.fields -%}
        , {{ field.name }}
{%- endfor -%}
        , false
    );
}

static inline error_t async_send_{{ msg.canon_name }}({{ msg.args | send_params }}) {
    return __do_send_{{ msg.canon_name }}(__ch
{%- for field in msg.args.fields -%}
        , {{ field.name }}
{%- endfor -%}
        , true
    );
}
#endif

{%- if msg.attrs.type == "call" %}
#define {{ msg.canon_name | upper }}_REPLY_MSG (({{ interface.name | upper }}_INTERFACE << 8) | MSG_REPLY_FLAG | {{ msg.attrs.id }})
#define {{ msg.canon_name | upper }}_REPLY_INLINE_LEN ({{ msg.rets | inline_len }})
#define {{ msg.canon_name | upper }}_REPLY_HEADER \
    ( ({{ msg.canon_name | upper }}_REPLY_MSG        << MSG_TYPE_OFFSET) \
{%- if msg.rets.page -%}
    | MSG_PAGE_PAYLOAD  \
{%- endif -%}
{%- if msg.rets.channel -%}
    | MSG_CHANNEL_PAYLOAD  \
{%- endif -%}
    | ({{ msg.canon_name | upper }}_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET) \
    )

#if !defined(KERNEL)
static inline error_t __do_send_{{ msg.canon_name }}_reply({{ msg.rets | send_params }}, bool __async) {
    struct message m;
    m.header = {{ msg.canon_name | upper }}_REPLY_HEADER;
{% for field in msg.rets.fields %}
{%- if field.type == "smallstring" %}
    strcpy_s((char *) &m.payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ field.name }}, SMALLSTRING_LEN_MAX, {{ field.name }});
{%- else %}
    m.payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ field.name }} = {{ field.name }};
{%- endif %}
{%- endfor %}
    error_t err = __async ? ipc_async_send(__ch, &m) : ipc_send(__ch, &m);
    if (err != OK) {
        return err;
    }
    return OK;
}

static inline error_t send_{{ msg.canon_name }}_reply({{ msg.rets | send_params }}) {
    return __do_send_{{ msg.canon_name }}_reply(__ch
{%- for field in msg.rets.fields -%}
        , {{ field.name }}
{%- endfor -%}
        , false
    );
}

static inline error_t async_send_{{ msg.canon_name }}_reply({{ msg.rets | send_params }}) {
    return __do_send_{{ msg.canon_name }}_reply(__ch
{%- for field in msg.rets.fields -%}
        , {{ field.name }}
{%- endfor -%}
        , true
    );
}

static inline error_t call_{{ msg.canon_name }}({{ msg | call_params }}) {
    struct message m;
    m.header = {{ msg.canon_name | upper }}_HEADER;
{% for field in msg.args.fields %}
{%- if field.type == "smallstring" %}
    strcpy_s((char *) &m.payloads.{{ interface.name }}.{{ msg.name }}.{{ field.name }}, SMALLSTRING_LEN_MAX, {{ field.name }});
{%- else %}
    m.payloads.{{ interface.name }}.{{ msg.name }}.{{ field.name }} = {{ field.name }};
{%- endif %}
{%- endfor %}
{%- if msg.rets.page %}
    set_page_base({{ msg.rets.page.name }}_base);
{%- endif %}
    error_t err;
    if ((err = ipc_call(__ch, &m, &m)) != OK)
        return err;
{%- for field in msg.rets.inlines %}
{%- if field.type == "smallstring" %}
    strcpy_s((char *) {{ field.name }}, SMALLSTRING_LEN_MAX, &m.payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ field.name }});
{%- else %}
    *{{ field.name }} = m.payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ field.name }};
{%- endif %}
{%- endfor %}
{%- if msg.rets.channel %}
    *{{ msg.rets.channel.name }} = m.payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ msg.rets.channel.name }};
{%- endif %}
{%- if msg.rets.page %}
    set_page_base(0);
    *{{ msg.rets.page.name }} = PAGE_PAYLOAD_ADDR(m.payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ msg.rets.page.name }});
    *{{ msg.rets.page.name }}_num = PAGE_ORDER(m.payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ msg.rets.page.name }});
{%- endif %}
    return OK;
}
#endif
{%- endif %} {# msg.attrs.type == "call" #}
{%- endfor %} {# for msg in interface.messages #}
{%- endfor %} {# for interface in interfaces #}
"""

PAYLOAD_TEMPLATE = """\
#ifndef __RESEA_IDL_PAYLOADS_H__
#define __RESEA_IDL_PAYLOADS_H__

{%- for interface in interfaces %}
{%- for msg in interface.messages %}
struct {{ msg.canon_name }}_payload {
{%- if msg.args.page %}
    page_t {{ msg.args.page.name }};
{%- else %}
    page_t __unused_page;
{%- endif %}
{%- if msg.args.channel %}
    cid_t {{ msg.args.channel.name }};
{%- else %}
    cid_t __unused_channel;
{%- endif %}
    uint8_t _padding[12];
{%- for field in msg.args.inlines %}
    {{ field.type | resolve_type }} {{ field.name }};
{%- endfor %}
} PACKED;

{%- if msg.attrs.type == "call" %}
struct {{ msg.canon_name }}_reply_payload {
{%- if msg.rets.page %}
    page_t {{ msg.rets.page.name }};
{%- else %}
    page_t __unused_page;
{%- endif %}
{%- if msg.rets.channel %}
    cid_t {{ msg.rets.channel.name }};
{%- else %}
    cid_t __unused_channel;
{%- endif %}
    uint8_t _padding[12];
{%- for field in msg.rets.inlines %}
    {{ field.type | resolve_type }} {{ field.name }};
{%- endfor %}
} PACKED;
{%- endif %}
{%- endfor %}
{%- endfor %}

#define IDL_MESSAGE_PAYLOADS \\
{%- for interface in interfaces %}
    union { \\
{%- for msg in interface.messages %}
    struct {{ msg.canon_name }}_payload {{ msg.name }}; \\
{%- if msg.attrs.type == "call" %}
    struct {{ msg.canon_name }}_reply_payload {{ msg.name }}_reply; \\
{%- endif %}
{%- endfor %}
    } {{ interface.name }}; \\
{%- endfor %}

#endif
"""

SERVER_STUBS = """\
{%- for interface in interfaces %}
{%- for msg in interface.messages %}
static error_t handle_{{ msg.canon_name }}(struct message *m) {
{%- for field in msg.args.fields %}
    // TODO: {{ field.type | resolve_type }} {{ field.name }} = m->payloads.{{ interface.name }}.{{ msg.name }}.{{ field.name }};
{%- endfor %}
    UNIMPLEMENTED();
    m->header = {{ msg.canon_name | upper }}_REPLY_HEADER;
{%- for field in msg.rets.fields %}
    // TODO: m->payloads.{{ interface.name }}.{{ msg.name }}_reply.{{ field.name }} = ;
{%- endfor %}
}
{% endfor %}
{%- endfor %}

static error_t process_message(struct message *m) {
    switch (MSG_TYPE(m->header)) {
{%- for interface in interfaces %}
{%- for msg in interface.messages %}
    case {{ msg.canon_name | upper }}_MSG: return handle_{{ msg.canon_name }}(m);
{%- endfor %}
{%- endfor %}
    }
    return ERR_UNEXPECTED_MESSAGE;
}
"""

TEMPLATE = f"""\
#ifndef __RESEA_IDL_H__
#define __RESEA_IDL_H__

#include <message.h>

// Declare internally-used functions instead of including somewhat large header
// fies.
void set_page_base(page_base_t page_base);
error_t ipc_send(cid_t ch, struct message *m);
error_t ipc_async_send(cid_t ch, struct message *m);
error_t ipc_call(cid_t ch, struct message *m, struct message *r);
char *strcpy_s(char *dst, size_t dst_len, const char *src);
void *memcpy(void *dst, const void *src, size_t len);

//
//  Client-side stubs.
//
{CLIENT_STUBS}

#if 0
//
//  Server-side stubs: copy and paste into the your server!
//
{SERVER_STUBS}
#endif

#endif
"""

builtin_types = {
    "int8":        { "name": "int8_t",        "size": "sizeof(int8_t)" },
    "int16":       { "name": "int16_t",       "size": "sizeof(int16_t)" },
    "int32":       { "name": "int32_t",       "size": "sizeof(int32_t)" },
    "int64":       { "name": "int64_t",       "size": "sizeof(int64_t)" },
    "uint8":       { "name": "uint8_t",       "size": "sizeof(uint8_t)" },
    "uint16":      { "name": "uint16_t",      "size": "sizeof(uint16_t)" },
    "uint32":      { "name": "uint32_t",      "size": "sizeof(uint32_t)" },
    "uint64":      { "name": "uint64_t",      "size": "sizeof(uint64_t)" },
    "intmax":      { "name": "intmax_t",      "size": "sizeof(intmax_t)" },
    "uintmax":     { "name": "uintmax_t",     "size": "sizeof(uintmax_t)" },
    "pid":         { "name": "pid_t",         "size": "sizeof(pid_t)" },
    "tid":         { "name": "tid_t",         "size": "sizeof(tid_t)" },
    "cid":         { "name": "cid_t",         "size": "sizeof(cid_t)" },
    "channel":     { "name": "cid_t",         "size": "sizeof(cid_t)" },
    "uintptr":     { "name": "uintptr_t",     "size": "sizeof(uintptr_t)" },
    "size":        { "name": "size_t",        "size": "sizeof(size_t)" },
    "page":        { "name": "page_t",        "size": "sizeof(page_t)" },
    "page_base":   { "name": "page_base_t",   "size": "sizeof(page_base_t)" },
    "char":        { "name": "char",          "size": "sizeof(char)" },
    "smallstring": { "name": "smallstring_t", "size": "SMALLSTRING_LEN_MAX" },
}

user_defined_types = {}

# Resolves a type name to a corresponding builtin type.
def resolve_type(type_name):
    if type_name in builtin_types:
        return builtin_types[type_name]["name"]
    if type_name not in user_defined_types:
        raise InvalidIDLError(f"Invalid type name: '{type_name}'")
    return user_defined_types[type_name] + "_t"

def inline_len(params):
    sizes = []
    for field in params["inlines"]:
        typename = field["type"]
        while typename in user_defined_types:
            typename = user_defined_types[typename]
        sizes.append(builtin_types[typename]["size"])
    if len(sizes) == 0:
        return "0"
    else:
        return " + ".join(sizes)

def call_params(m):
    params = ["cid_t __ch"]
    for arg in m["args"]["fields"]:
        if arg["type"] == "smallstring":
            params.append(f"const {resolve_type('char')}* {arg['name']}")
        else:
            params.append(f"{resolve_type(arg['type'])} {arg['name']}")
    for ret in m["rets"]["fields"]:
        if ret["type"] == "page":
            params.append(f"{resolve_type('page_base')} {ret['name']}_base")
    for ret in m["rets"]["fields"]:
        if ret["type"] == "page":
            params.append(f"{resolve_type('uintptr')} *{ret['name']}")
            params.append(f"{resolve_type('size')} *{ret['name']}_num")
        else:
            params.append(f"{resolve_type(ret['type'])} *{ret['name']}")
    return ", ".join(params)

def send_params(fields):
    params = ["cid_t __ch"]
    for field in fields["fields"]:
        if field["type"] == "smallstring":
            params.append(f"const {resolve_type('char')}* {field['name']}")
        else:
            params.append(f"{resolve_type(field['type'])} {field['name']}")
    return ", ".join(params)

def genstub(interfaces):
    global user_defined_types
    for interface in interfaces:
        for typedef in interface["types"]:
            user_defined_types[typedef["name"]] = typedef["alias_of"]

    renderer = jinja2.Environment()
    renderer.filters["resolve_type"] = resolve_type
    renderer.filters["inline_len"] = inline_len
    renderer.filters["call_params"] = call_params
    renderer.filters["send_params"] = send_params
    return {
        "stubs": renderer.from_string(TEMPLATE).render(interfaces=interfaces),
        "payloads": renderer.from_string(PAYLOAD_TEMPLATE)
            .render(interfaces=interfaces)
    }

def main():
    parser = argparse.ArgumentParser(description="The C stub generator.")
    parser.add_argument("idl_file", help="The IDL file.")
    parser.add_argument("-o", dest="out_file", help="The output file.")
    args = parser.parse_args()

    ast = Lark(GRAMMAR, start="idl").parse(open(args.idl_file).read())
    idl = IdlTransformer().transform(ast)
    stub = genstub(idl)

    with open(Path(args.out_file), "w") as f:
        f.write(stub["stubs"])
    with open(Path(args.out_file).parent / "resea_idl_payloads.h", "w") as f:
        f.write(stub["payloads"])

if __name__ == "__main__":
    main()
