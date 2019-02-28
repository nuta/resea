from lark import Lark, Transformer

GRAMMAR = """
ID: /[a-zA-Z_][a-zA-Z0-9_]*/
VALUE: /[a-zA-Z0-9_-][a-zA-Z0-9_-]*/
COMMENT: /\/\/.*/

%import common.WS
%ignore WS
%ignore COMMENT

idl: interface
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

class IdlTransformer(Transformer):
    def message(self, child):
        attrs, name, req, res = child
        return {
            "attrs": attrs,
            "name": str(name),
            "req": req,
            "res": res,
        }

    def type(self, child):
        name, alias_of = child
        return {
            "name": str(name),
            "alias_of": str(alias_of),
        }

    def interface(self, child):
        attrs, name, types, messages = child
        return {
            "name": str(name),
            "attrs": attrs,
            "types": types,
            "messages": messages,
        }

    tokens_to_tuple = lambda _, ts: tuple(map(str, ts))

    types = list
    messages = list
    attrs = dict
    attr = tokens_to_tuple
    args = list
    arg = tokens_to_tuple
    defs = list
    idl = lambda _, child: child[0]

class InvalidIdlException(Exception):
    pass

def validate_idl(idl):
    used_method_ids = []

    if "id" not in idl["attrs"]:
        raise InvalidIdlException("The interface ID must be specified.")

    MSG_ATTRS = ["id", "type"]
    MSG_TYPES = ["rpc", "upcall"]
    for m in idl["messages"]:
        if "id" not in m["attrs"]:
            raise InvalidIdlException("The interface ID must be specified.")
        if m["attrs"]["type"] not in MSG_TYPES:
            raise InvalidIdlException("The message type must be one of: {MSG_TYPES}")
        for attr_name in m["attrs"].keys():
            if attr_name not in MSG_ATTRS:
                raise InvalidIdlException("Unknown message attribute: `{attr_name}'")

        method_id = int(m["attrs"]["id"])
        if method_id in used_method_ids:
            raise InvalidIdlException(f"The duplicated method ID: {method_id} (name={m['name']})")
        if method_id >= 128:
            raise InvalidIdlException(f"Too large method ID: {method_id} (name={m['name']})")
        used_method_ids.append(method_id)

def parse_idl(idl):
    ast = Lark(GRAMMAR, start="idl").parse(idl)
    idl = IdlTransformer().transform(ast)
    validate_idl(idl)
    return idl

