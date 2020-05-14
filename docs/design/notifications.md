# Notifications
While the sending and receiving a message are blocking operations and it works
pretty well in most cases, sometimes you will need a asynchronous way to send a message.

*Notification* is asynchronous (non-blocking) IPC mechanism which simply updates
(bitwise-ORing) the notifications field of the destination task. When a task
tries to receive a message, the kernel first checks pending (unreceived)
notifications, and if exists, the kernel returns them as a message.

Note that **notification IPC is just a bitfield update** (just like *signals*
in UNIX): it's impossible to determine how many times a same notification has
been notified.

## Why we need notifications?
Let's say that you're desiging a TCP/IP server and underlying device drivers:

```
              send a message
             when packet arrives
   Device  ----------------------->  TCP/IP
   Driver                            Server
     ^                                |
     |                                |
     +--------------------------------+
       send a message to emit packets
```

It looks an intuitive approach, however, what if the device
driver tries sending a network packet when the TCP/IP server is trying to
send a message to the driver? It would cause a deadlock because IPC operations
are blocking.

The most important thing when you're writing a Resea application is:
**never have two task send messages each other** as described in
[QNX's IPC documentation](http://www.qnx.co.jp/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_sys_arch%2Fipc.html).

How can we avoid deadlocks in Resea? This is where notifications comes into play.

## Notify & Pull Pattern
Instead of sending message each other, when TCP/IP wants to send some data to
the driver, it *notifies* the driver asynchronously that there's pending data.
When the device driver receives the notification, it pulls the pending data
via message passing:

```
            2. request and receive
             the sending packet
   Device  ----------------------->  TCP/IP
   Driver                            Server
     ^                                .
     .                                .
     ..................................
         1. notify that new data is
          available asynchronously
```
