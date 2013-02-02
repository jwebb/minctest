#include <stdlib.h>
#include <stack.h>

// Not intended as an example of good data structure implementation...

typedef struct stack_element {
	int value;
	struct stack_element* next;
} stack_element;

void stack_init(simple_stack* stack)
{
	stack->top = NULL;
}

void stack_free(simple_stack* stack)
{
	while (stack->top)
		stack_pop(stack);
}

void stack_push(simple_stack* stack, int value)
{
	stack_element* e = malloc(sizeof(stack_element));
	e->value = value;
	e->next = stack->top;
	stack->top = e;
}

int stack_pop(simple_stack* stack)
{
	int value = stack->top->value;
	stack_element* new_top = stack->top->next;
	free(stack->top);
	stack->top= new_top;
	return value;
}

int stack_height(simple_stack* stack)
{
	int h = 0;
	for (stack_element* e = stack->top; e; e = e->next)
		++h;
	return h;
}
