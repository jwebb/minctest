MinCTest
========

Copyright (c) 2013 Correct Code Ltd.

MinCTest is a simple and concise unit testing framework for C. It depends
only on the standard library, so it should be portable to any modern compiler.

It consists of a single header and a single source file, which are expected to
be copied into the host project. A minimal example follows. See
`example/stack_test.c` for a more complete one.

Example
-------

```c
#include <minctest.h>

void my_first_test()
{
    mc_int_eq(1 + 1, 2);
    mc_deny(0 == 1);
}

void my_second_test()
{
    if (0 == 1)
        mc_fail("Oh noes!");
}

void my_test_suite()
{
    // The suite function is executed repeatedly, running just one test
    // each time.

    my_per_test_setup_code();

    mc_test(my_first_test);
    mc_test(my_second_test);

    my_per_test_teardown_code();
}

int main(int argc, char** argv)
{
    mc_suite(my_test_suite);
    return mc_run(argc, argv);
}
```

Command line
------------

Currently mc_run accepts a single command line option: `--verbose` prints the name
of every test as it is run.

Assertions
----------

The complete set of available assertions to date is:

```c
mc_assert(expr)
mc_deny(expr)
mc_fail(reason)
mc_int_eq(int1, int2)
mc_int_ne(int1, int2)
mc_int_gt(int1, int2)
mc_int_lt(int1, int2)
mc_int_ge(int1, int2)
mc_int_le(int1, int2)
mc_uint64_eq(int1, int2)
mc_uint64_ne(int1, int2)
mc_uint64_gt(int1, int2)
mc_uint64_lt(int1, int2)
mc_uint64_ge(int1, int2)
mc_uint64_le(int1, int2)
mc_str_eq(str1, str2)
mc_str_ne(str1, str2)
mc_bytes_eq(bytes1, bytes2, length)
mc_bytes_ne(bytes1, bytes2, length)
```

Allocation and cleanup
----------------------

MinCTest provides `void* mc_cleanup(void (*cleaner_fn)(void*), void* data)` to
record a cleanup action that will always execute after the current test finishes.
It also provides the shortcut `void* mc_alloc(size_t size)`, which is equivalent
to `mc_cleanup(free, calloc(1, size))`.

Hex dumps
---------

The function `const char* mc_hexdump(size_t length, void* data)` is used internally,
but is also available to client code as a debugging convenience.

UNIX signals
------------

MinCTest has some support for handling signals, if it's built with `MC_UNIX`
defined. In this case, signals such as `SEGV` will simply cause the current test
to fail. This may or may not be useful, depending on how messed up the program
state gets...
