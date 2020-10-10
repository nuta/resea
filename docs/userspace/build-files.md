# Build Files

Add `build.mk` in the server's directory:

```mk
# build.mk

# The server name. It must satisfy /[a-zA-Z_][a-zA-Z0-9_]*/
name := ps2_keyboard
# A short description.
description := PS/2 Keyboard Driver
# Object files.
objs-y += main.o
# Library dependencies.
libs-y += driver
```

If you'd like to add build configuration, add `Kconfig` file to the directory:

```
menu "ps2_keyboard - PS/2 Keyboard Driver"
    # PS2_KEYBOARD_SERVER is set if this `ps2_keyboard` is enabled in the config.
	depends on PS2_KEYBOARD_SERVER

    config PRINT_PERIODICALLY
        bool "Print a message every second"
endmenu
```

See [Kconfig Language](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
for details.
