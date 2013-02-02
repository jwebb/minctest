typedef struct simple_stack {
	struct stack_element* top;
} simple_stack;

void stack_init(simple_stack* stack);
void stack_free(simple_stack* stack);
void stack_push(simple_stack* stack, int value);
int stack_pop(simple_stack* stack);
int stack_height(simple_stack* stack);
