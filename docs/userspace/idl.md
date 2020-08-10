# Interface Definiton Language (IDL)
Since we plan to support multiple programming languages (C and Rust), for
interoperability, we use our own Interface Definition Language (IDL).

Message definitions are automatically generated from the IDL files.

Let's a take a look at an example:

```
namespace fs {
    rpc open(path: str) -> (handle: handle);
    rpc close(handle: handle) -> ();
    rpc read(handle: handle, offset: offset, len: size) -> (data: bytes);
}
```
