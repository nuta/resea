# Interfaces
Serializing and deserializing messages are error-prone and painful. Resea provides
auto-generated *IPC stubs* to make it easy to send and receive messages.

Every server provides at least one service which definied as *interface*,
a set of messages that implementors must accept and respond.

This file defines the interfaces in our own domain specific language. We
haven't yet documented the language specification, but it should be pretty
straightforward to understand. We use this definition to generate IPC stubs.

Generated stubs can be found in `resea::idl` module.

## Adding a new interface
Let's add our `rand` interface:
```
[id(99)] // A interface ID must be unique.
interface rand {
    // A method ID must be unique in the interface.
    [id(1), type(call)]
    get_random_value() -> (value: uint32)

    [id(2), type(oneway)]
    add_randomness(randomness: uint32) -> ()
}
```

Here we define two methods in the `rand` interface: `get_random_value` and
`add_randomness`. 

`get_random_value` returns a random value to the caller and `add_randomness`
injects random value into the entropy pool.
