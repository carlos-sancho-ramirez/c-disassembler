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

int stack_is_empty(struct Stack *stack) {
    return stack->word_count == 0;
}

int push_undefined_in_stack(struct Stack *stack) {
    const unsigned int index = stack->word_count >> 3;
    if (stack->word_count == stack->allocated_word_count) {
        stack->allocated_word_count += STACK_VALUES_GRANULARITY;
        stack->values = realloc(stack->values, stack->allocated_word_count);
        stack->defined_and_relative = realloc(stack->defined_and_relative, (stack->allocated_word_count >> 3) + STACK_DEFINED_AND_RELATIVE_GRANULARITY);
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
        stack->values = realloc(stack->values, stack->allocated_word_count);
        stack->defined_and_relative = realloc(stack->defined_and_relative, (stack->allocated_word_count >> 3) + STACK_DEFINED_AND_RELATIVE_GRANULARITY);
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
        stack->values = realloc(stack->values, stack->allocated_word_count);
        stack->defined_and_relative = realloc(stack->defined_and_relative, (stack->allocated_word_count >> 3) + STACK_DEFINED_AND_RELATIVE_GRANULARITY);
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
    return stack->values[--stack->word_count];
}

int top_is_defined_in_stack(struct Stack *stack) {
    unsigned int index = stack->word_count - 1;
    return stack->defined_and_relative[index >> 3] & (1 << ((index & 0x07) * 2));
}

int top_is_defined_and_relative_in_stack(struct Stack *stack) {
    unsigned int index = stack->word_count - 1;
    uint16_t mask = 3 << ((index & 0x07) * 2);
    return (stack->defined_and_relative[index >> 3] & mask) == mask;
}
