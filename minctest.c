/*
This is MinCTest, copyright (c) 2013 Correct Code Ltd.

This file comes from https://github.com/jwebb/minctest.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <minctest.h>
#include <stdarg.h>

#ifdef MC_UNIX
#include <signal.h>
#include <setjmp.h>
static jmp_buf signal_jmp_buf;
#endif

typedef enum test_result {
	STARTED, PASSED, FAILED, CRASHED
} test_result;

typedef struct test {
	const char* name;
	int index;
	void (*fn)();
	test_result result;
	const char* file;
	int line;
	char* reason;
	struct test* next;
} test;

typedef struct suite {
	const char* name;
	void (*fn)();
	test* first_test;
	test* last_test;
	int test_count;
	test setup;
	int failures;
	struct suite* next;
} suite;

typedef struct cleanup {
	void (*fn)(void *);
	void* data;
	struct cleanup* next;
} cleanup;

static bool verbose = false;
static bool both_outputs = false;
static suite* first_suite = NULL;
static suite* last_suite = NULL;
static suite* current_suite = NULL;
static test* current_test = NULL;
static int test_index = 0;
static cleanup* cleanups = NULL;

static void parse_args(int argc, char** argv);
static void run_suite(suite* s);
static void run_with_signals_caught(suite* s);
static void print_failures(suite* s);
static void print_failed_test(test* t);
static void free_data();

// asprintf is a GNU extension, so provide our own primitive version as a backup...
#ifdef __GNUC__
static int local_vasprintf(char** ret, const char* fmt, va_list ap)
{
	return vasprintf(ret, fmt, ap);
}
#else
static int local_vasprintf(char** ret, const char* fmt, va_list ap)
{
	va_list ap2;
	va_copy(ap2, ap);
	char dummy[1];
	int count = vsnprintf(dummy, 1, fmt, ap);
	char* buf = malloc(count + 1);
	int r = vsnprintf(buf, count + 1, fmt, ap2);
	*ret = buf;
	return r;
}
#endif

static int local_asprintf(char** ret, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vasprintf(ret, fmt, ap);
	va_end(ap);
	return r;
}

int mc_run(int argc, char** argv)
{
	parse_args(argc, argv);
	fprintf(stderr, "\n");

	for (suite* s = first_suite; s; s = s->next) {
		run_suite(s);
	}

	fprintf(stderr, "\n\n");

	int run = 0;
	int failed = 0;
	for (suite* s = first_suite; s; s = s->next) {
		run += s->test_count;
		failed += s->failures;
		if (s->failures)
			print_failures(s);
	}

	fprintf(stderr, "\n%s - %d of %d tests failed.\n\n",
			(failed ? "FAILURE" : "SUCCESS"), failed, run);

	free_data();
	return (failed ? EXIT_FAILURE : EXIT_SUCCESS);
}

void mc_suite_internal(const char* name, void (*fn)())
{
	suite* s = calloc(1, sizeof(suite));
	s->name = name;
	s->fn = fn;
	s->setup.name = "(setup/teardown)";
	s->setup.result = PASSED;
	if (last_suite)
		last_suite->next = s;
	if (!first_suite)
		first_suite = s;
	last_suite = s;
}

void mc_test_internal(const char* name, void (*fn)())
{
	if (current_suite) {
		// We only run one test per iteration of the suite
		if (test_index == current_suite->test_count) {
			if (verbose)
				fprintf(stderr, "  Test: %s... ", name);
			if (both_outputs)
				printf("\n# Test: %s\n", name);

			test* t = calloc(1, sizeof(test));
			t->name = name;
			t->fn = fn;
			t->result = STARTED;
			t->index = test_index;
			if (current_suite->last_test)
				current_suite->last_test->next = t;
			if (!current_suite->first_test)
				current_suite->first_test = t;
			current_suite->last_test = t;

			test* setup = current_test;
			current_test = t;
			fn();
			switch (t->result) {
				case STARTED:
					t->result = PASSED;
					fprintf(stderr, (verbose ? "PASS\n" : "."));
					break;
				case FAILED:
					++current_suite->failures;
					fprintf(stderr, (verbose ? "FAIL\n" : "x"));
					break;
				case CRASHED:
					++current_suite->failures;
					fprintf(stderr, (verbose ? "CRASH!\n" : "#"));
					break;
				case PASSED:
					// Shouldn't happen
					break;
			}
			current_test = setup;
		}
		++test_index;
	}
}

bool mc_assert_internal(bool pass, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (!pass) {
		char* reason;
		local_vasprintf(&reason, fmt, ap);
		if (current_test) {
			current_test->result = FAILED;
			current_test->reason = reason;
		} else if (current_suite) {
			current_suite->setup.result = FAILED;
			current_suite->setup.reason = reason;
		}
	}
	return pass;
}

void mc_checkpoint_internal(const char* file, int line)
{
	if (current_test) {
		current_test->file = file;
		current_test->line = line;
	}
}

const char* mc_hexdump(size_t len, const void* bytes)
{
	int stop = (len > 0x1000 ? 0x1000 : len);
	int bufsize = (stop + 15) / 16 * (4 + 1 + 48 + 16 + 1) + 5;
	char* buf = mc_alloc(bufsize);
	char row[17];
	int pos = 0;
	row[16] = 0;

	for (int i = 0; i < stop; i += 16) {
		pos += sprintf(buf + pos, "%04x ", i);
		for (int j = 0; j < 16; ++j) {
			if (i + j < stop) {
				char c = ((char*) bytes)[i + j];
				pos += sprintf(buf + pos, "%02x ", c & 0xff);
				row[j] = (c > ' ' && c <= '~' ? c : '.');
			} else {
				pos += sprintf(buf + pos, "   ");
				row[j] = ' ';
			}
		}
		pos += sprintf(buf + pos, "%s\n", row);
	}
	if (stop < len) {
		pos += sprintf(buf + pos, "...\n");
	}

	return buf;
}

void* mc_alloc(size_t size)
{
	void* data = calloc(1, size);
	if (!data)
		return NULL;

	return mc_cleanup(free, data);
}

void* mc_cleanup(void (*cleanup_fn)(void*), void* data)
{
	cleanup* node = malloc(sizeof(cleanup));
	if (!node)
		return NULL;

	node->fn = cleanup_fn;
	node->data = data;
	node->next = cleanups;
	cleanups = node;
	return data;
}

static void run_cleanups()
{
	cleanup* curr = cleanups;
	while (curr) {
		cleanup* next = curr->next;
		(curr->fn)(curr->data);
		free(curr);
		curr = next;
	}
	cleanups = NULL;
}

static void parse_args(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--verbose"))
			verbose = true;
		else if (!strcmp(argv[i], "--both-outputs"))
			both_outputs = true;
		else
			fprintf(stderr, "Warning: unrecognised command line argument: %s\n", argv[i]);
	}
}

static void run_suite(suite* s)
{
	if (verbose)
		fprintf(stderr, "Suite: %s\n", s->name);
	if (both_outputs)
		printf("\n\n# Suite: %s\n", s->name);

	current_suite = s;
	current_test = &s->setup;

	int crash_count = 0;
	do {
		test_index = 0;
		run_with_signals_caught(s);
		run_cleanups();

		if (s->setup.result != PASSED) {
			++s->failures;
			return;
		}

		++s->test_count;

		// Having run the suite, test_index ends up with the number of
		// tests found in the suite (which may be less than the actual
		// number if a test crashed), and test_count with the number that have
		// run to completion.
		if (test_index < s->test_count)
			++crash_count;
		if (crash_count > 5)
			break;
	} while (test_index != s->test_count);
}

static void print_failures(suite* s)
{
	fprintf(stderr, "Failed: %s\n", s->name);
	print_failed_test(&s->setup);
	for (test* t = s->first_test; t; t = t->next) {
		print_failed_test(t);
	}
}

static void print_failed_test(test* t)
{
	switch (t->result) {
		case FAILED:
		case CRASHED:
			fprintf(stderr, "  - %s, %s:%d: %s\n", t->name, t->file, t->line, t->reason);
			break;
		case STARTED:
		case PASSED:
			// Didn't fail
			break;
	}
}

static void free_data()
{
	suite* s = first_suite;
	while (s) {
		test* t = s->first_test;
		while (t) {
			test* n = t->next;
			free(t->reason);
			free(t);
			t = n;
		}

		suite* n = s->next;
		free(s);
		s = n;
	}
}

#ifdef MC_UNIX
static void signal_handler(int sig) {
	// The literature suggests this 'often works', despite not being technically
	// supported. At worst, we crash anyway...
	siglongjmp(signal_jmp_buf, sig);
}

static char* name_signal(int sig)
{
	const char* name;
	char* out;
	switch (sig) {
		case SIGABRT: name = "SIGABRT"; break;
		case SIGFPE: name = "SIGFPE"; break;
		case SIGBUS: name = "SIGBUS"; break;
		case SIGSEGV: name = "SIGSEGV"; break;
		case SIGPIPE: name = "SIGPIPE"; break;
		case SIGSYS: name = "SIGSYS"; break;
		default:
			local_asprintf(&out, "Caught signal %d", sig);
			return out;
	}
	local_asprintf(&out, "Caught %s", name);
	return out;
}

static void set_signals(struct sigaction* sa)
{
	sigaction(SIGABRT, sa, NULL);
	sigaction(SIGFPE, sa, NULL);
	sigaction(SIGBUS, sa, NULL);
	sigaction(SIGSEGV, sa, NULL);
	sigaction(SIGPIPE, sa, NULL);
	sigaction(SIGSYS, sa, NULL);
}

static void run_with_signals_caught(suite* s)
{
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sa.sa_mask = 0;
	sa.sa_flags = SA_RESETHAND;
	set_signals(&sa);

	int sig = sigsetjmp(signal_jmp_buf, 1);
	if (!sig) {
		s->fn();
	} else {
		// We just longjmp'd here due to a signal
		if (current_test) {
			current_test->result = CRASHED;
			current_test->reason = name_signal(sig);
			++s->failures;
			fprintf(stderr, (verbose ? "CRASH!\n" : "#"));
		}
	}

	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	set_signals(&sa);
}
#else
static void run_with_signals_caught(suite* s)
{
	// Maybe one day we will catch crashes on other OSs...
	s->fn();
}
#endif
