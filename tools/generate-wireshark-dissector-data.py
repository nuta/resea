#!/usr/bin/env python3
import argparse
import json
import jinja2

TEMPLATE = """\
{%- for interface in interfaces %}
{%- for msg in interface.messages %}
{%- for field in msg.args.inlines %}
proto.fields.{{ interface.name }}_{{ msg.name }}_{{ field.name }} = ProtoField.{{ field.type | proto_field }}("resea.payloads.{{ interface.name }}.{{ msg.name }}.{{ field.name }}", "{{ field.name }}" {{ field.type | proto_base }});
{%- endfor %}
{%- endfor %}
{%- endfor %}

resea_messages = {
{%- for interface in interfaces %}
{%- for msg in interface.messages %}
    [{{ msg.id | hex }}] = {
        interface_name = "{{ interface.name }}",
        name = "{{ interface.name }}.{{ msg.name }}",
        fields = {
{%- for field in msg.args.inlines %}
            { name="{{ field.name }}", proto=proto.fields.{{ interface.name }}_{{ msg.name }}_{{ field.name }}, offset={{ field.offset }}, len={{ field.len }} },
{%- endfor %}
        }
    },
{%- endfor %}
{%- endfor %}
}
"""

def proto_field(type_):
    return {
        "int8":    "int8",
        "int16":   "int16",
        "int32":   "int32",
        "int64":   "int64",
        "uint8":   "uint8",
        "uint16":  "uint16",
        "uint32":  "uint32",
        "uint64":  "uint64",
        "intmax":  "int64",
        "uintmax": "uint64",
        "uintptr": "uint64",
        "paddr":   "uint64",
        "size":    "uint64",
        "cid":     "int32",
        "handle":  "int32",
        "bool":    "bool",
        "char":    "string",
        "string":  "string",
    }[type_]

def proto_base(type_):
    base = {
        "int8":    "DEC",
        "int16":   "DEC",
        "int32":   "DEC",
        "int64":   "DEC",
        "uint8":   "DEC",
        "uint16":  "DEC",
        "uint32":  "DEC",
        "uint64":  "DEC",
        "intmax":  "DEC",
        "uintmax": "DEC",
        "uintptr": "HEX",
        "paddr":   "HEX",
        "size":    "HEX",
        "cid":     "DEC",
        "handle":  "DEC",
        "bool":    None,
        "char":    None,
        "string":  None,
    }[type_]

    if base is not None:
        return f", base.{base}"
    else:
        return ""

def main():
    parser = argparse.ArgumentParser(
        description="Generates a IDL table for the Wireshark dissector.")
    parser.add_argument("idl_json", help="The parsed IDL JSON file.")
    args = parser.parse_args()

    interfaces = json.load(open(args.idl_json))
    renderer = jinja2.Environment()
    renderer.filters["hex"] = hex
    renderer.filters["proto_field"] = proto_field
    renderer.filters["proto_base"] = proto_base
    print(renderer.from_string(TEMPLATE).render(interfaces=interfaces))

if __name__ == "__main__":
    main()
