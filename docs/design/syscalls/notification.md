# Notification
While synchronous (blocking) IPC works pretty fine in most cases, sometimes you
may want asynchronous (non-blocking) IPC. For example, the kernel converts
interrupts into messages but sending it should not block.

Notification is a simple solution just like *signal* in Unix:

- `error_t notify(cid_t cid, notification_t notification)`
  - Sends a notification to the channel. It simply updates the notification
    field in the destination channel (by OR'ing `notification`).
  - If a receiver thread already waits for a message on the destination channel,
    the kernel sends a message as `NOTIFICATION_MSG` to it.
  -  It never blocks (asynchronous).
  - `notification_t` is `uint32_t`.
