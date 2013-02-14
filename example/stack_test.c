#include <stdlib.h>
#include <minctest.h>
#include <stack.h>

// Our fixture will be set up for each test
static simple_stack target;

// Now for some tests. These function names appear in the test report.
static void should_start_empty()
{
	mc_int_eq(stack_height(&target), 0);
}

static void should_change_size()
{
	// Assertions return from the current function if they fail, and log the
	// source file, line number, and the expected and actual source
	// expressions and runtime values.
	stack_push(&target, 1);
	mc_int_eq(stack_height(&target), 1);
	stack_push(&target, 1);
	mc_int_eq(stack_height(&target), 2);
	stack_pop(&target);
	mc_int_eq(stack_height(&target), 1);
	stack_pop(&target);
	mc_int_eq(stack_height(&target), 0);
}

static void should_return_added_element()
{
	stack_push(&target, 5);
	mc_int_eq(stack_pop(&target), 5);
}

// Build our tests into a suite
void stack_tests()
{
	// Setup code
	stack_init(&target);

	// We declare the tests that we wrote above. Note that stack_tests will be
	// called repeatedly, and only one of these tests will actually be executed
	// on each iteration. This is to allow the setup/teardown code to run for
	// each test.
	mc_test(should_start_empty);
	mc_test(should_change_size);
	mc_test(should_return_added_element);

	// Teardown code
	stack_free(&target);
}

// Now, some example failing tests
static void example_equal_failure()
{
	const char* a = "test string";
	const char* b = "non-matching string";
	mc_str_eq(a, b);
}

static void example_explicit_failure()
{
	mc_fail("This test fails deliberately");
}

static void example_signal()
{
	// On POSIX systems, this will raise SIGABRT, which will be caught and
	// logged by the test framework. We only try this if signal handling is
	// enabled.
#ifdef MC_UNIX
	mc_checkpoint();
	abort();
#endif
}

static void example_hex_dump()
{
	mc_bytes_eq(20, "This\tis the\0 first!\n", "This is\r\nthe second!");
}

void negative_tests()
{
	mc_test(example_equal_failure);
	mc_test(example_explicit_failure);
	mc_test(example_signal);
	mc_test(example_hex_dump);
}

int main(int argc, char** argv)
{
	// First declare some suites
	mc_suite(stack_tests);
	mc_suite(negative_tests);

	// Then run them
	return mc_run(argc, argv);
}
