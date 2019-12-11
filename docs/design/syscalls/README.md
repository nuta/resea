# System Calls
All system calls are only essential ones for message passing:

- `int open(void)`
  - Creates a new channel. Returns its channel ID or a negated `error_t` value.
- `error_t close(cid_t ch)`
  - Destroys a channel.
- `error_t link(cid_t ch1, cid_t ch2)`
  - Links two channels. Messages from `ch1` (resp. `ch2`) will be sent to `ch2`
    (resp. `ch1`).
- `error_t transfer(cid_t src, cid_t dst)`
  - Transfer messages from the channel linked to `src` to `dst` channels.
- `error_t ipc(cid_t ch, uint32_t ops)`
  - Performs synchronous IPC operations. `ops` can be send, receive, or both
    of them (so-called *Remote Procedure Call*).
- `error_t notify(cid_t cid, notification_t notification)`
  - Sends a notification to the channel (asynchronous IPC).
