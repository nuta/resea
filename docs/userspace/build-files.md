# Build Files

Add `build.mk` in the server's directory. Replace `<name>` with appropriate name.

```mk
# build.mk
name := <name>
obj-y += main.o
```

YOu also need to add a config in `servers/Kconfig`:

```
# servers/Kconfig

config <name>_SERVER
    tristate "<description>"
```

See [Kconfig Language](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
for details.

If you're familiar with Linux kernel development, you may notice that this example
uses `tristate`.

The `M` state is usually used for kernel modules, however, in Resea,
it builds and embeds the server into the OS image but it's not automatically started
(you need to start it from the shell).
