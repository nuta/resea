# System Calls
- `open() -> ch`
- `close(ch)`
- `ipc(ch, accept, send_flags, types, label, m0, ..., m4) ->
    (sent_from, recv_flags, types, label, m0, ..., m4) or (error)`
    - **send_flags:** send, recv, reply
    - **recv_flags:** notification, error
    - **types:** inline, page, channel
    - **label:** interface type, method ID
- `notify(ch, op, arg0, arg1)`
    - **op:** atomic add, atomic dec, atomic and/or
- `link(ch1, ch2)`
- `transfer(src, dst)`
- `duplicate(ch) -> ch`
