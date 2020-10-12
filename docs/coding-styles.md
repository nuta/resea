# Conding Style Guides
Please follow this style guide for consistency. Because a well-written coding
style is boring to read, we are not strict with the coding styles.

## Line Length
80 characters.

## C Style Guides

### Indentation
4 spaces.

### Functions
```c
void handle_message(struct message *m) {
}

static inline struct toooooooooo_loooooooong *function(int a, int b, int c,
                                                       int d, int e) {
}

static void func(
    const struct very_long_name *a,
    const struct very_long_name *b
) {
}
```

### Switch
```c
switch (m.type) {
    case FS_READ_MSG:
        return_value = fs_read();
        break;
    case FS_WRITE_MSG: {
        const void *data = m.fs_write.data;
        fs_write(data);
        break;
    }
}
```

### Conditions
```c
if (very_looooooooong_variable1 == very_looooooooong_variable2
    || very_looooooooong_variable1 != very_looooooooong_variable3) {
    stmt();
}
```

### Function Calls
```c
do_something(a, b, c, d,
             e, f, g, h);
```

### Too Long Expressions
```c
int result = very_looooooooong_variable1 + very_looooooooong_variable2
             + very_looooooooong_variable1;
```

or

```c
int result = very_looooooooong_variable1 + very_looooooooong_variable2
    + very_looooooooong_variable1;
```

### Macros
```c
#define MAX(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) __a = (a);                                               \
        __typeof__(b) __b = (b);                                               \
        (__a > __b) ? __a : __b;                                               \
    })
```

### Don't omit curly braces for single-line bodies in `if`, `while`, ...
This rule prevents some nasty bugs (and sometimes [security vulnerabilities](https://www.imperialviolet.org/2014/02/22/applebug.html)).
```c
// Bad
if (expr)
    stmt();

// OK
if (expr) {
    stmt();
}
```

## Python Style Guides
Please follow [PIP-8](https://www.python.org/dev/peps/pep-0008/).

## Rust Style Guides
Use [rustfmt](https://github.com/rust-lang/rustfmt).
