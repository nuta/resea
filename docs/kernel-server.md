# Kernel Server

```
kill_current.kill_current()
print.putchar()

process.create() -> pid
process.destroy(pid)
process.add_pager(pid, pager_ch, base, len, rwx)

thread.spawn(pid, start, stack, arg) -> tid
thread.destroy(pid)
thread.wait(tid)

timer.listen()
timer.interrupt_notification()

interrupt.listen()
io.allow_ioports(base, len)
io.map_physical_pages(page)

pager.fill_request(base, len) -> (page)

process.exit_notification(pid)
thread.exit_notification(tid)
```