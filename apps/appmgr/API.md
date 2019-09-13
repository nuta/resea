API
===


```c
void *mem_alloc(size_t bytes);
void mem_free(void *ptr);

void console_write(const char *fmt, ...);
size_t console_read(char *buf, size_t buf_len, console_read_options_t options /* NOECHO | RAW */);

file_t fs_open(const char *path, open_mode_t mode);
void fs_close(file_t file);
size_t fs_read(file_t file, void *buf, size_t buf_len);
size_t fs_write(file_t file, void *buf, size_t buf_len);
size_t fs_seek(file_t file, size_t offset, seek_type_t type);
size_t fs_tell(file_t file);

proc_t proc_spawn(const char *path, const char **argv);
void proc_join(proc_t proc);
void proc_kill(proc_t proc);

thread_t thread_spawn(void (*start)(void *arg), void *arg);
void thread_join(thread_t thread);
void thread_kill(thread_t thread);

// Network
// GUI
// Date & Time
// Random
// String & Data Structures
```
