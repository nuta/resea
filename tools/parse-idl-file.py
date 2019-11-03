import argparse
import json
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

        for msg in messages:
            msg["id"] = int(attrs["id"]) << 8 | int(msg["attrs"]["id"])
            offset = 32
            for field in msg["args"]["fields"]:
                size = builtin_sizes[field["type"]]
                field["offset"] = offset
                field["len"] = size
                offset += size

        return {
            "name": str(name),
            "attrs": attrs,
            "types": types,
            "messages": messages,
        }

builtin_sizes = {
    "int8":    1,
    "int16":   2,
    "int32":   4,
    "int64":   8,
    "uint8":   1,
    "uint16":  2,
    "uint32":  4,
    "uint64":  8,
    "bool":    1,
    "char":    1,

    # These payloads are not included in the inline payload.
    "channel": 0,
    "page":    0,

    # FIXME: Arch-dependent types.
    "size":    8,
    "paddr":   8,
    "uintptr": 8,
    "intmax":  8,
    "uintmax": 8,
    "cid":     4,
    "handle":  4,
    "string": 128,
}

def main():
    parser = argparse.ArgumentParser(description=
        "Parses a IDL file and outputs a structured JSON file.")
    parser.add_argument("idl_file", help="The IDL file.")
    parser.add_argument("outfile", help="The output file.")
    args = parser.parse_args()

    ast = Lark(GRAMMAR, start="idl").parse(open(args.idl_file).read())
    idl = IdlTransformer().transform(ast)
    json.dump(idl, open(args.outfile, "w"), indent=2)

if __name__ == "__main__":
    main()
