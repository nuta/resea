# Unit Testing
`libs/unittest` provides a very primitive unit testing framework for libraries
(in `libs`) and userspace applications (in `servers`). It's useful if you're
writing a somewhat complicated function.

This framework enables some attractive characteristics and features:

- The framework compiles tests into a normal userspace application for the your
  development environment (e.g. macOS).
- Since the testing program is a native application, it is super-fast and
  you can use your favorite debugging tools like LLDB!
- [Undefined Behavior Sanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) and [Address Santizer](https://clang.llvm.org/docs/AddressSanitizer.html) are enabled by default.

## Caveats
- You can't use system calls (e.g. message passing) from the testing environment.
- `main()` in your Resea application won't be called.

## Writing Tests
```c
#include <unittest.h>

int add(int a, int b) {
    return a + b;
}

TEST("1 + 1 equals to 2") {
    TEST_EXPECT_EQ(add(1, 1), 2);
}
```

### Macros
- `TEST("description")`: Use this macro to define a unit testing function.
- `TEST_EXPECT_EQ(a, b)`: Checks if `a == b` holds.
- `TEST_EXPECT_NE(a, b)`: Checks if `a != b` holds.
- `TEST_EXPECT_LT(a, b)`: Checks if `a < b` holds.
- `TEST_EXPECT_LE(a, b)`: Checks if `a <= b` holds.
- `TEST_EXPECT_GT(a, b)`: Checks if `a > b` holds.
- `TEST_EXPECT_GE(a, b)`: Checks if `a >= b` holds.

## How to Run Tests
```
$ make -j8 -f libs/unittest/unittest.mk TARGET=servers/apps/test
```
