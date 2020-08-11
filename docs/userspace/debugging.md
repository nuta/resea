# Debugging
We don't have rich debugging features yet. Use printf macros. Good luck!

## How to deal with dead locks in IPC
Even if you don't use locks like *mutex* (note that we don't provide such
a thing), your program could be blocked forever by an IPC operation.

The common case is that both your program and the destination task are trying
to send a message to each other. You can check it by `ps` command
in the kernel debugger:

```
kdebug> ps
#1 vm: state=blocked, src=0
#2 display: state=blocked, src=0
#3 e1000: state=blocked, src=6
  senders:
    - #6 tcpip
#4 ps2kbd: state=blocked, src=0
#5 shell: state=blocked, src=0
#6 tcpip: state=blocked, src=3
  senders:
    - #3 e1000
#7 webapi: state=blocked, src=0
```

Notice that `e1000` and `tcpip` are blocked and they're sending to
the other server.
