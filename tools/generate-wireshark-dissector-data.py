#!/usr/bin/env python3
import argparse
import json
import jinja2

TEMPLATE = """\
{%- for interface in interfaces %}
{%- for msg in interface.messages %}
{%- for field in msg.args.fields %}
proto.fields.{{ interface.name }}_{{ msg.name }}_{{ field.name }} = ProtoField.{{ field.type | proto_field }}("resea.payloads.{{ interface.name }}.{{ msg.name }}.{{ field.name }}", "{{ field.name }}");
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
{%- for field in msg.args.fields %}
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
        "bool":    "bool",
        "char":    "string",
    }[type_]

def main():
    parser = argparse.ArgumentParser(
        description="Generates a IDL table for the Wireshark dissector.")
    parser.add_argument("idl_json", help="The parsed IDL JSON file.")
    args = parser.parse_args()

    interfaces = json.load(open(args.idl_json))
    renderer = jinja2.Environment()
    renderer.filters["hex"] = hex
    renderer.filters["proto_field"] = proto_field
    print(renderer.from_string(TEMPLATE).render(interfaces=interfaces))

if __name__ == "__main__":
    main()
