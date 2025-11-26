#include "stack.h"
#include <stdlib.h>

#define STACK_DEFINED_AND_RELATIVE_GRANULARITY 4
#define STACK_VALUES_GRANULARITY (8 * STACK_DEFINED_AND_RELATIVE_GRANULARITY)

void initialize_stack(struct Stack *stack) {
	stack->allocated_word_count = 0;
	stack->word_count = 0;
	stack->values = NULL;
	stack->defined_and_relative = NULL;
}

void clear_stack(struct Stack *stack) {
	if (stack->allocated_word_count > 0) {
		free(stack->values);
		free(stack->defined_and_relative);
	}

	initialize_stack(stack);
}

int stack_is_empty(const struct Stack *stack) {
	return stack->word_count == 0;
}

int push_undefined_in_stack(struct Stack *stack) {
	const unsigned int index = stack->word_count >> 3;
	if (stack->word_count == stack->allocated_word_count) {
		stack->allocated_word_count += STACK_VALUES_GRANULARITY;
		stack->values = realloc(stack->values, stack->allocated_word_count * 2);
		stack->defined_and_relative = realloc(stack->defined_and_relative, stack->allocated_word_count >> 2);
		if (!stack->values || !stack->defined_and_relative) {
			return 1;
		}
	}

	stack->defined_and_relative[index] &= ~(1 << ((stack->word_count & 0x07) * 2));
	stack->word_count++;
	return 0;
}

int push_in_stack(struct Stack *stack, uint16_t value) {
	const unsigned int index = stack->word_count >> 3;
	if (stack->word_count == stack->allocated_word_count) {
		stack->allocated_word_count += STACK_VALUES_GRANULARITY;
		stack->values = realloc(stack->values, stack->allocated_word_count * 2);
		stack->defined_and_relative = realloc(stack->defined_and_relative, stack->allocated_word_count >> 2);
		if (!stack->values || !stack->defined_and_relative) {
			return 1;
		}
	}

	stack->values[stack->word_count] = value;
	stack->defined_and_relative[index] |= 1 << ((stack->word_count & 0x07) * 2);
	stack->defined_and_relative[index] &= ~(1 << ((stack->word_count & 0x07) * 2 + 1));
	stack->word_count++;
	return 0;
}

int push_relative_in_stack(struct Stack *stack, uint16_t value) {
	const unsigned int index = stack->word_count >> 3;
	if (stack->word_count == stack->allocated_word_count) {
		stack->allocated_word_count += STACK_VALUES_GRANULARITY;
		stack->values = realloc(stack->values, stack->allocated_word_count * 2);
		stack->defined_and_relative = realloc(stack->defined_and_relative, stack->allocated_word_count >> 2);
		if (!stack->values || !stack->defined_and_relative) {
			return 1;
		}
	}

	stack->values[stack->word_count] = value;
	stack->defined_and_relative[index] |= 3 << ((stack->word_count & 0x07) * 2);
	stack->word_count++;
	return 0;
}

uint16_t pop_from_stack(struct Stack *stack) {
	if (stack->word_count > 0) {
		return stack->values[--stack->word_count];
	}
	else {
		return 0;
	}
}

int is_defined_in_stack_from_top(const struct Stack *stack, unsigned int count) {
	if (stack->word_count > count) {
		unsigned int index = stack->word_count - count - 1;
		return stack->defined_and_relative[index >> 3] & (1 << ((index & 0x07) * 2));
	}
	else {
		return 0;
	}
}

int is_defined_relative_in_stack_from_top(const struct Stack *stack, unsigned int count) {
	if (stack->word_count > count) {
		unsigned int index = stack->word_count - count - 1;
		uint16_t mask = 3 << ((index & 0x07) * 2);
		return (stack->defined_and_relative[index >> 3] & mask) == mask;
	}
	else {
		return 0;
	}
}

int top_is_defined_in_stack(const struct Stack *stack) {
	return is_defined_in_stack_from_top(stack, 0);
}

int top_is_defined_relative_in_stack(const struct Stack *stack) {
	return is_defined_relative_in_stack_from_top(stack, 0);
}

uint16_t get_from_top(const struct Stack *stack, unsigned int count) {
	return (stack->word_count > count)? stack->values[stack->word_count - count - 1] : 0;
}

int copy_stack(struct Stack *target_stack, const struct Stack *source_stack) {
	const int word_count = source_stack->word_count;
	int index;

	if (target_stack->allocated_word_count > 0) {
		free(target_stack->values);
		free(target_stack->defined_and_relative);
	}

	target_stack->word_count = word_count;
	target_stack->allocated_word_count = ((source_stack->word_count + (STACK_VALUES_GRANULARITY - 1)) / STACK_VALUES_GRANULARITY) * STACK_VALUES_GRANULARITY;
	target_stack->values = malloc(target_stack->allocated_word_count * 2);
	target_stack->defined_and_relative = malloc(target_stack->allocated_word_count >> 2);
	if (!target_stack->values || !target_stack->defined_and_relative) {
		return 1;
	}

	for (index = 0; index < word_count; index++) {
		target_stack->values[index] = source_stack->values[index];
	}

	for (index = 0; index < ((word_count + 7) >> 3); index++) {
		target_stack->defined_and_relative[index] = source_stack->defined_and_relative[index];
	}

	return 0;
}

void merge_stacks(struct Stack *stack, const struct Stack *other_stack) {
	const int this_count = stack->word_count;
	const int other_count = other_stack->word_count;
	int this_index;
	if (this_count <= other_count) {
		for (this_index = 0; this_index < this_count; this_index++) {
			const int this_defined_and_relative = (stack->defined_and_relative[this_index >> 3] >> ((this_index & 7) * 2)) & 3;
			if (this_defined_and_relative & 1) {
				const int other_index = this_index + other_count - this_count;
				const int other_defined_and_relative = (stack->defined_and_relative[other_index >> 3] >> ((other_index & 7) * 2)) & 3;
				if (this_defined_and_relative == other_defined_and_relative) {
					const uint16_t this_value = stack->values[this_index];
					const uint16_t other_value = other_stack->values[other_index];
					if (this_value != other_value) {
						stack->defined_and_relative[this_index >> 3] &= ~(3 << ((other_index & 7) * 2));
					}
				}
				else {
					stack->defined_and_relative[this_index >> 3] &= ~(3 << ((other_index & 7) * 2));
				}
			}
		}
	}
	else {
		for (this_index = 0; this_index < this_count - other_count; this_index++) {
			stack->defined_and_relative[this_index >> 3] &= ~(3 << ((this_index & 7) * 2));
		}

		for (this_index = this_count - other_count; this_index < this_count; this_index++) {
			const int this_defined_and_relative = (stack->defined_and_relative[this_index >> 3] >> ((this_index & 7) * 2)) & 3;
			if (this_defined_and_relative & 1) {
				const int other_index = this_index + other_count - this_count;
				const int other_defined_and_relative = (stack->defined_and_relative[other_index >> 3] >> ((other_index & 7) * 2)) & 3;
				if (this_defined_and_relative == other_defined_and_relative) {
					const uint16_t this_value = stack->values[this_index];
					const uint16_t other_value = other_stack->values[other_index];
					if (this_value != other_value) {
						stack->defined_and_relative[this_index >> 3] &= ~(3 << ((other_index & 7) * 2));
					}
				}
				else {
					stack->defined_and_relative[this_index >> 3] &= ~(3 << ((other_index & 7) * 2));
				}
			}
		}
	}
}

int changes_on_merging_stacks(const struct Stack *stack, const struct Stack *other_stack) {
	const int this_count = stack->word_count;
	const int other_count = other_stack->word_count;
	int this_index;

	if (this_count <= other_count) {
		for (this_index = 0; this_index < this_count; this_index++) {
			const int this_defined_and_relative = (stack->defined_and_relative[this_index >> 3] >> ((this_index & 7) * 2)) & 3;
			if (this_defined_and_relative & 1) {
				const int other_index = this_index + other_count - this_count;
				const int other_defined_and_relative = (stack->defined_and_relative[other_index >> 3] >> ((other_index & 7) * 2)) & 3;
				if (this_defined_and_relative != other_defined_and_relative || stack->values[this_index] != other_stack->values[other_index]) {
					return 1;
				}
			}
		}
	}
	else {
		for (this_index = this_count - other_count; this_index < this_count; this_index++) {
			const int this_defined_and_relative = (stack->defined_and_relative[this_index >> 3] >> ((this_index & 7) * 2)) & 3;
			if (this_defined_and_relative & 1) {
				const int other_index = this_index + other_count - this_count;
				const int other_defined_and_relative = (stack->defined_and_relative[other_index >> 3] >> ((other_index & 7) * 2)) & 3;
				if (this_defined_and_relative != other_defined_and_relative || stack->values[this_index] != other_stack->values[other_index]) {
					return 1;
				}
			}
		}
	}

	return 0;
}
