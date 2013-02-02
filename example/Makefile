test: stack_test
	./stack_test

stack_test: stack.c stack_test.c ../minctest.c stack.h ../minctest.h
	cc -g -o stack_test -std=c99 -DMC_UNIX -I.. -I. stack.c stack_test.c ../minctest.c

clean:
	rm -rf stack_test stack_test.dSYM

.PHONY: test clean
