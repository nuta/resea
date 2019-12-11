# Adding New Features
I don't know why you're going to add new features to the *micro*-kernel, you
have two options to do that:

- **Implement a new interface in the kernel server:**
  Update `interfaces.idl` and implement the newly added method (or interface)
  in the kernel server (see `kernel_server_main`).
- **Add a system call:** Despite we strongly discourage it but it is a simple
  way to do. `syscall_handler` is where the kernel handles system calls. Add
  your new system call code into it.
