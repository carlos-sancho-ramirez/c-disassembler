#ifndef _STACK_H_
#define _STACK_H_

#include <stdint.h>

struct Stack {
	unsigned int allocated_word_count;
	unsigned int word_count;
	uint16_t *values;
	uint16_t *defined_and_relative;
};

void initialize_stack(struct Stack *stack);
void clear_stack(struct Stack *stack);

int stack_is_empty(struct Stack *stack);
int push_undefined_in_stack(struct Stack *stack);
int push_in_stack(struct Stack *stack, uint16_t value);
int push_relative_in_stack(struct Stack *stack, uint16_t value);
uint16_t pop_from_stack(struct Stack *stack);
int top_is_defined_in_stack(struct Stack *stack);
int top_is_defined_and_relative_in_stack(struct Stack *stack);

int copy_stack(struct Stack *target_stack, const struct Stack *source_stack);
void merge_stacks(struct Stack *stack, const struct Stack *other_stack);
int changes_on_merging_stacks(const struct Stack *stack, const struct Stack *other_stack);

#endif /* _STACK_H_ */
