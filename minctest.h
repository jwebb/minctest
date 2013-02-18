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

#ifndef INC_MINCTEST_H
#define INC_MINCTEST_H

#include <stdbool.h>
#include <string.h>

/* Runs the tests, writes a report and returns the exit code. Call this after
declaring some suites. */
int mc_run(int argc, char** argv);

/* Declares (but does not execute) a test suite. A suite a is a function
containing per-test setup code, a set of test declarations, and per-test
teardown code, in that order. It will be called repeatedly, with one test in
turn being executed on each iteration. */
#define mc_suite(fn) mc_suite_internal(#fn, fn)
void mc_suite_internal(const char* name, void (*fn)());

/* Declares a test. A rtest is a function containing assertions. */
#define mc_test(fn) mc_test_internal(#fn, fn)
void mc_test_internal(const char* name, void (*fn)());

/* Test assertions. A failed assertion returns from current function. */
#define mc_assert(expr) \
	do { \
		mc_checkpoint(); \
		if (!mc_assert_internal((expr), "Expected true: %s, but was false", #expr)) \
			return; \
	} while (0)

#define mc_deny(expr) \
	do { \
		mc_checkpoint(); \
		if (!mc_assert_internal(!(expr), "Expected false: %s, but was true", #expr)) \
			return; \
	} while (0)

#define mc_fail(reason) \
	do { \
		mc_checkpoint(); \
		mc_assert_internal(0, "%s", reason); \
		return; \
	} while (0)

#define mc_general_cmp(type, left, right, op, msg) \
	do { \
		mc_checkpoint(); \
		type l = (left); \
		type r = (right); \
		if (!mc_assert_internal(op, msg, #left, #right, l, r)) \
			return; \
	} while (0)

#define mc_int_eq(left, right) mc_general_cmp(int, left, right, l == r, "Expected %s == %s, but %d != %d")
#define mc_int_ne(left, right) mc_general_cmp(int, left, right, l != r, "Expected %s !== %s, but %d == %d")
#define mc_int_gt(left, right) mc_general_cmp(int, left, right, l > r, "Expected %s > %s, but %d <= %d")
#define mc_int_lt(left, right) mc_general_cmp(int, left, right, l < r, "Expected %s < %s, but %d >= %d")
#define mc_int_ge(left, right) mc_general_cmp(int, left, right, l >= r, "Expected %s >= %s, but %d < %d")
#define mc_int_le(left, right) mc_general_cmp(int, left, right, l <= r, "Expected %s <= %s, but %d > %d")
#define mc_uint64_eq(left, right) mc_general_cmp(uint64_t, left, right, l == r, "Expected %s == %s, but %lld != %lld")
#define mc_uint64_ne(left, right) mc_general_cmp(uint64_t, left, right, l != r, "Expected %s !== %s, but %lld == %lld")
#define mc_uint64_gt(left, right) mc_general_cmp(uint64_t, left, right, l > r, "Expected %s > %s, but %lld <= %lld")
#define mc_uint64_lt(left, right) mc_general_cmp(uint64_t, left, right, l < r, "Expected %s < %s, but %lld >= %lld")
#define mc_uint64_ge(left, right) mc_general_cmp(uint64_t, left, right, l >= r, "Expected %s >= %s, but %lld < %lld")
#define mc_uint64_le(left, right) mc_general_cmp(uint64_t, left, right, l <= r, "Expected %s <= %s, but %lld > %lld")
#define mc_str_eq(left, right) mc_general_cmp(const char*, left, right, !strcmp(l, r), "Expected %s == %s, but '%s' != '%s'")
#define mc_str_ne(left, right) mc_general_cmp(const char*, left, right, strcmp(l, r), "Expected %s != %s, but '%s' == '%s'")

#define mc_bytes_eq(length, left, right) \
	do { \
		mc_checkpoint(); \
		size_t len = (length); \
		const char* l = (left); \
		const char* r = (right); \
		if (!mc_assert_internal(!memcmp(l, r, len), "Expected %s == %s, but left:\n\n%s\n  vs right:\n\n%s", \
				#left, #right, mc_hexdump(len, l), mc_hexdump(len, r))) \
			return; \
	} while (0)

#define mc_bytes_ne(length, left, right) \
	do { \
		mc_checkpoint(); \
		const char* l = (left); \
		const char* r = (right); \
		if (!mc_assert_internal(memcmp(l, r, (length)), "Expected %s != %s, but equal'", #left, #right)) \
			return; \
	} while (0)

bool mc_assert_internal(bool pass, const char* fmt, ...);

/* Notes the current source location, reported in event of e.g. segfault. */
#define mc_checkpoint() mc_checkpoint_internal(__FILE__, __LINE__)
void mc_checkpoint_internal(const char* file, int line);

/* Returns an mc_alloc'd string containing a hex dump of the given bytes. */
const char* mc_hexdump(size_t len, const void* bytes);

/* Allocates memory, which will be automatically free'd at the end of the
current test. Aborts if the memory cannot be allocated. */
void* mc_alloc(size_t size);

#endif
