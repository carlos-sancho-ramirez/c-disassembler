#include "stack.h"
#include <stdlib.h>

#define STACK_BYTES_IN_REL_PER_PAGE 4
#define STACK_BYTES_PER_PAGE (STACK_BYTES_IN_REL_PER_PAGE * 16)
#define STACK_BYTES_IN_DNM_PER_PAGE (STACK_BYTES_IN_REL_PER_PAGE * 4)

/* This must be at least STACK_BYTES_PER_PAGE / 2 */
#define STACK_SHRINK_TOP_THRESHOLD STACK_BYTES_PER_PAGE

#include <assert.h>

void initialize_stack(struct Stack *stack) {
	stack->allocated_pages = 0;
	stack->top = 0;
	stack->data = NULL;
	stack->defined_and_merged = NULL;
	stack->relative = NULL;
}

void clear_stack(struct Stack *stack) {
	if (stack->allocated_pages > 0) {
		free(stack->data);
		free(stack->defined_and_merged);
		free(stack->relative);
	}

	initialize_stack(stack);
}

int is_defined_in_stack_from_top(const struct Stack *stack, unsigned int count) {
	const unsigned int allocated_bytes = stack->allocated_pages * STACK_BYTES_PER_PAGE;
	const unsigned int data_index = (stack->top + count) * 2;
	if (data_index >= allocated_bytes) {
		return 0;
	}
	else {
		const unsigned int mask = 5 << (data_index % (4 * sizeof(packed_data_t)) * 2);
		return (stack->defined_and_merged[data_index / (4 * sizeof(packed_data_t))] & mask) == mask;
	}
}

int is_defined_absolute_in_stack_from_top(const struct Stack *stack, unsigned int count) {
	return is_defined_in_stack_from_top(stack, count) && (stack->relative[(stack->top + count) / (8 * sizeof(packed_data_t))] & 1 << (stack->top + count) % (8 * sizeof(packed_data_t))) == 0;
}

int is_defined_relative_in_stack_from_top(const struct Stack *stack, unsigned int count) {
	return is_defined_in_stack_from_top(stack, count) && stack->relative[(stack->top + count) / (8 * sizeof(packed_data_t))] & 1 << (stack->top + count) % (8 * sizeof(packed_data_t));
}

int top_is_defined_in_stack(const struct Stack *stack) {
	return is_defined_in_stack_from_top(stack, 0);
}

int top_is_defined_absolute_in_stack(const struct Stack *stack) {
	return is_defined_absolute_in_stack_from_top(stack, 0);
}

int top_is_defined_relative_in_stack(const struct Stack *stack) {
	return is_defined_relative_in_stack_from_top(stack, 0);
}

uint16_t get_from_top(const struct Stack *stack, unsigned int count) {
	const unsigned int allocated_bytes = stack->allocated_pages * STACK_BYTES_PER_PAGE;
	const unsigned int data_index = (stack->top + count) * 2;
	uint16_t result = stack->data[data_index + 1] & 0xFF;
	result = (result << 8) + (stack->data[data_index] & 0xFF);
	return result;
}

static int add_new_pages_at_start(struct Stack *stack, unsigned int count) {
	unsigned int offset;
	int i;

	stack->allocated_pages += count;
	stack->data = realloc(stack->data, stack->allocated_pages * STACK_BYTES_PER_PAGE);
	stack->defined_and_merged = realloc(stack->defined_and_merged, stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE);
	stack->relative = realloc(stack->relative, stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE);
	if (!stack->data || !stack->defined_and_merged || !stack->relative) {
		return 1;
	}

	offset = count * STACK_BYTES_PER_PAGE;
	for (i = stack->allocated_pages * STACK_BYTES_PER_PAGE - 1; i >= offset; i--) {
		stack->data[i] = stack->data[i - offset];
	}

	offset = count * STACK_BYTES_IN_DNM_PER_PAGE / sizeof(packed_data_t);
	for (i = stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE / sizeof(packed_data_t) - 1; i >= offset; i--) {
		stack->defined_and_merged[i] = stack->defined_and_merged[i - offset];
	}

	offset = count * STACK_BYTES_IN_REL_PER_PAGE / sizeof(packed_data_t);
	for (i = stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE / sizeof(packed_data_t) - 1; i >= offset; i--) {
		stack->relative[i] = stack->relative[i - offset];
	}

	stack->top += (count * STACK_BYTES_PER_PAGE) / 2;
	return 0;
}

static int add_new_pages_at_end(struct Stack *stack, unsigned int count) {
	unsigned int end;
	int i;

	stack->allocated_pages += count;
	stack->data = realloc(stack->data, stack->allocated_pages * STACK_BYTES_PER_PAGE);
	stack->defined_and_merged = realloc(stack->defined_and_merged, stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE);
	stack->relative = realloc(stack->relative, stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE);
	if (!stack->data || !stack->defined_and_merged || !stack->relative) {
		return 1;
	}

	end = stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE / sizeof(packed_data_t);
	for (i = (stack->allocated_pages - count) * STACK_BYTES_IN_DNM_PER_PAGE / sizeof(packed_data_t); i < end; i++) {
		stack->defined_and_merged[i] = 0;
	}

	return 0;
}

static int remove_pages_at_start(struct Stack *stack, unsigned int count) {
	if (count >= stack->allocated_pages) {
		clear_stack(stack);
	}
	else {
		unsigned int end;
		unsigned int offset;
		int i;

		assert(stack->top >= (count * STACK_BYTES_PER_PAGE) / 2);

		end = stack->allocated_pages * STACK_BYTES_PER_PAGE;
		offset = count * STACK_BYTES_PER_PAGE;
		for (i = offset; i < end; i++) {
			stack->data[i - offset] = stack->data[i];
		}

		end = (stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE) / sizeof(packed_data_t);
		offset = (count * STACK_BYTES_IN_DNM_PER_PAGE) / sizeof(packed_data_t);
		for (i = offset; i < end; i++) {
			stack->defined_and_merged[i - offset] = stack->defined_and_merged[i];
		}

		end = (stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE) / sizeof(packed_data_t);
		offset = (count * STACK_BYTES_IN_REL_PER_PAGE) / sizeof(packed_data_t);
		for (i = offset; i < end; i++) {
			stack->relative[i - offset] = stack->relative[i];
		}

		stack->allocated_pages -= count;
		stack->data = realloc(stack->data, stack->allocated_pages * STACK_BYTES_PER_PAGE);
		stack->defined_and_merged = realloc(stack->defined_and_merged, stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE);
		stack->relative = realloc(stack->relative, stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE);
		if (!stack->data || !stack->defined_and_merged || !stack->relative) {
			return 1;
		}

		stack->top -= (count * STACK_BYTES_PER_PAGE) / 2;
	}

	return 0;
}

int push_undefined_in_stack(struct Stack *stack) {
	int error_code;
	if (stack->top == 0 && (error_code = add_new_pages_at_start(stack, 1))) {
		return error_code;
	}

	stack->top--;
	stack->defined_and_merged[(stack->top >> 1) / sizeof(packed_data_t)] &= ~(15 << ((stack->top << 1) % (sizeof(packed_data_t) * 4)) * 2);
	return 0;
}

int push_in_stack(struct Stack *stack, uint16_t value) {
	int error_code;
	if (stack->top == 0 && (error_code = add_new_pages_at_start(stack, 1))) {
		return error_code;
	}

	stack->top--;
	stack->defined_and_merged[(stack->top >> 1) / sizeof(packed_data_t)] &= ~(10 << ((stack->top << 1) % (sizeof(packed_data_t) * 4)) * 2);
	stack->defined_and_merged[(stack->top >> 1) / sizeof(packed_data_t)] |= 5 << ((stack->top << 1) % (sizeof(packed_data_t) * 4) * 2);
	stack->relative[(stack->top >> 3) / sizeof(packed_data_t)] &= ~(1 << (stack->top % (sizeof(packed_data_t) * 8)));
	stack->data[stack->top * 2] = value & 0xFF;
	stack->data[stack->top * 2 + 1] = (value >> 8) & 0xFF;
	return 0;
}

int push_relative_in_stack(struct Stack *stack, uint16_t value) {
	int error_code;
	if (stack->top == 0 && (error_code = add_new_pages_at_start(stack, 1))) {
		return error_code;
	}

	stack->top--;
	stack->defined_and_merged[(stack->top >> 1) / sizeof(packed_data_t)] &= ~(10 << ((stack->top << 1) % (sizeof(packed_data_t) * 4)) * 2);
	stack->defined_and_merged[(stack->top >> 1) / sizeof(packed_data_t)] |= 5 << ((stack->top << 1) % (sizeof(packed_data_t) * 4) * 2);
	stack->relative[(stack->top >> 3) / sizeof(packed_data_t)] |= 1 << (stack->top % (sizeof(packed_data_t) * 8));
	stack->data[stack->top * 2] = value & 0xFF;
	stack->data[stack->top * 2 + 1] = (value >> 8) & 0xFF;
	return 0;
}

uint16_t pop_from_stack(struct Stack *stack) {
	const int allocated_bytes = stack->allocated_pages * STACK_BYTES_PER_PAGE;
	uint16_t result;

	if (stack->top * 2 == allocated_bytes) {
		return 0;
	}

	result = stack->data[stack->top * 2 + 1];
	result <<= 8;
	result += stack->data[stack->top * 2];
	stack->top++;

	if (stack->top >= STACK_SHRINK_TOP_THRESHOLD) {
		remove_pages_at_start(stack, 1); /* We cannot report any error here, as the return type is in use... */
	}

	return result;
}

int set_byte_in_stack_from_top(struct Stack *stack, unsigned int offset, unsigned char value) {
	const int byte_index = stack->top * 2 + offset;
	if (byte_index > stack->allocated_pages * STACK_BYTES_PER_PAGE) {
		const int required_extra_pages = (byte_index - stack->allocated_pages * STACK_BYTES_PER_PAGE + STACK_BYTES_PER_PAGE - 1) / STACK_BYTES_PER_PAGE;
		int error_code;

		if (required_extra_pages > 0 && (error_code = add_new_pages_at_end(stack, required_extra_pages))) {
			return error_code;
		}
	}

	stack->data[byte_index] = value;
	stack->defined_and_merged[byte_index / 4 / sizeof(packed_data_t)] &= ~(2 << byte_index % (4 * sizeof(packed_data_t)) * 2);
	stack->defined_and_merged[byte_index / 4 / sizeof(packed_data_t)] |= 1 << byte_index % (4 * sizeof(packed_data_t)) * 2;

	return 0;
}

int set_word_in_stack_from_top(struct Stack *stack, unsigned int offset, uint16_t value) {
	const int byte_index = stack->top * 2 + offset;
	if (byte_index > stack->allocated_pages * STACK_BYTES_PER_PAGE) {
		const int required_extra_pages = (byte_index - stack->allocated_pages * STACK_BYTES_PER_PAGE + STACK_BYTES_PER_PAGE - 1) / STACK_BYTES_PER_PAGE;
		int error_code;

		if (required_extra_pages > 0 && (error_code = add_new_pages_at_end(stack, required_extra_pages))) {
			return error_code;
		}
	}

	stack->data[byte_index] = value & 0xFF;
	stack->data[byte_index + 1] = (value >> 8) & 0xFF;
	stack->defined_and_merged[byte_index / 4 / sizeof(packed_data_t)] &= ~(2 << byte_index % (4 * sizeof(packed_data_t)) * 2);
	stack->defined_and_merged[byte_index / 4 / sizeof(packed_data_t)] |= 1 << byte_index % (4 * sizeof(packed_data_t)) * 2;
	stack->defined_and_merged[(byte_index + 1) / 4 / sizeof(packed_data_t)] &= ~(2 << (byte_index + 1) % (4 * sizeof(packed_data_t)) * 2);
	stack->defined_and_merged[(byte_index + 1) / 4 / sizeof(packed_data_t)] |= 1 << (byte_index + 1) % (4 * sizeof(packed_data_t)) * 2;
	if ((offset & 1) == 0) {
		stack->relative[byte_index / 16 / sizeof(packed_data_t)] &= ~(1 << byte_index % (16 * sizeof(packed_data_t)));
	}

	return 0;
}

int set_relative_word_in_stack_from_top(struct Stack *stack, unsigned int offset, uint16_t value) {
	const int byte_index = stack->top * 2 + offset;
	assert((offset & 1) == 0);

	if (byte_index > stack->allocated_pages * STACK_BYTES_PER_PAGE) {
		const int required_extra_pages = (byte_index - stack->allocated_pages * STACK_BYTES_PER_PAGE + STACK_BYTES_PER_PAGE - 1) / STACK_BYTES_PER_PAGE;
		int error_code;

		if (required_extra_pages > 0 && (error_code = add_new_pages_at_end(stack, required_extra_pages))) {
			return error_code;
		}
	}

	stack->data[byte_index] = value & 0xFF;
	stack->data[byte_index + 1] = (value >> 8) & 0xFF;
	stack->defined_and_merged[byte_index / 4 / sizeof(packed_data_t)] &= ~(2 << byte_index % (4 * sizeof(packed_data_t)) * 2);
	stack->defined_and_merged[byte_index / 4 / sizeof(packed_data_t)] |= 1 << byte_index % (4 * sizeof(packed_data_t) * 2);
	stack->defined_and_merged[(byte_index + 1) / 4 / sizeof(packed_data_t)] &= ~(2 << (byte_index + 1) % (4 * sizeof(packed_data_t)) * 2);
	stack->defined_and_merged[(byte_index + 1) / 4 / sizeof(packed_data_t)] |= 1 << (byte_index + 1) % (4 * sizeof(packed_data_t) * 2);
	stack->relative[byte_index / 16 / sizeof(packed_data_t)] |= 1 << byte_index % (16 * sizeof(packed_data_t));

	return 0;
}

void set_undefined_byte_in_stack_from_top(struct Stack *stack, unsigned int offset) {
	const int byte_index = stack->top * 2 + offset;
	if (byte_index < stack->allocated_pages * STACK_BYTES_PER_PAGE) {
		stack->defined_and_merged[byte_index / 4 / sizeof(packed_data_t)] &= ~(3 << byte_index % (4 * sizeof(packed_data_t)) * 2);
	}
}

void set_undefined_word_in_stack_from_top(struct Stack *stack, unsigned int offset) {
	set_undefined_byte_in_stack_from_top(stack, offset);
	set_undefined_byte_in_stack_from_top(stack, offset + 1);
}

int copy_stack(struct Stack *target_stack, const struct Stack *source_stack) {
	if (source_stack->allocated_pages == 0) {
		clear_stack(target_stack);
	}
	else {
		int index;

		if (target_stack->allocated_pages != source_stack->allocated_pages) {
			if (target_stack->allocated_pages > 0) {
				free(target_stack->data);
				free(target_stack->defined_and_merged);
				free(target_stack->relative);
			}

			target_stack->allocated_pages = source_stack->allocated_pages;
			target_stack->data = malloc(target_stack->allocated_pages * STACK_BYTES_PER_PAGE);
			target_stack->defined_and_merged = malloc(target_stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE);
			target_stack->relative = malloc(target_stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE);
			if (!target_stack->data || !target_stack->defined_and_merged || !target_stack->relative) {
				return 1;
			}
		}

		for (index = 0; index < source_stack->allocated_pages * STACK_BYTES_PER_PAGE; index++) {
			target_stack->data[index] = source_stack->data[index];
		}

		for (index = 0; index < source_stack->allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE / sizeof(packed_data_t); index++) {
			target_stack->defined_and_merged[index] = source_stack->defined_and_merged[index];
		}

		for (index = 0; index < source_stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE / sizeof(packed_data_t); index++) {
			target_stack->relative[index] = source_stack->relative[index];
		}

		target_stack->top = source_stack->top;
	}

	return 0;
}

int merge_stacks(struct Stack *stack, const struct Stack *other_stack) {
	int this_bytes = stack->allocated_pages * STACK_BYTES_PER_PAGE - stack->top * 2;
	int other_bytes = other_stack->allocated_pages * STACK_BYTES_PER_PAGE - other_stack->top * 2;
	int required_bytes = (this_bytes < other_bytes)? other_bytes : this_bytes;

	unsigned int new_allocated_pages;
	unsigned int new_top;
	unsigned char *new_data;
	packed_data_t *new_dnm;
	packed_data_t *new_rel;
	int this_relative;
	int other_relative;
	int i;

	while (required_bytes > 0) {
		const int this_defined_or_merged = required_bytes + stack->top * 2 - 1 < stack->allocated_pages * STACK_BYTES_PER_PAGE && stack->defined_and_merged[(required_bytes + stack->top * 2 - 1) / 4 / sizeof(packed_data_t)] & 3 << ((required_bytes + stack->top * 2 - 1) % (4 * sizeof(packed_data_t))) * 2;

		if (this_defined_or_merged) {
			break;
		}
		else {
			const int other_defined_or_merged = required_bytes + other_stack->top * 2 - 1 < other_stack->allocated_pages * STACK_BYTES_PER_PAGE && other_stack->defined_and_merged[(required_bytes + other_stack->top * 2 - 1) / 4 / sizeof(packed_data_t)] & 3 << ((required_bytes + other_stack->top * 2 - 1) % (4 * sizeof(packed_data_t))) * 2;
			if (other_defined_or_merged) {
				break;
			}
			else {
				required_bytes--;
			}
		}
	}

	if (required_bytes & 1) {
		required_bytes++;
	}

	new_allocated_pages = (required_bytes + STACK_BYTES_PER_PAGE - 1) / STACK_BYTES_PER_PAGE;
	new_top = (new_allocated_pages * STACK_BYTES_PER_PAGE - required_bytes) / 2;
	new_data = malloc(new_allocated_pages * STACK_BYTES_PER_PAGE);
	new_dnm = malloc(new_allocated_pages * STACK_BYTES_IN_DNM_PER_PAGE);
	new_rel = malloc(new_allocated_pages * STACK_BYTES_IN_REL_PER_PAGE);

	if (!new_data || !new_dnm || !new_rel) {
		return 1;
	}

	for (i = 0; i < required_bytes; i++) {
		const int this_index = stack->top * 2 + i;
		const int other_index = other_stack->top * 2 + i;
		const int new_index = new_top * 2 + i;
		const int this_defined = this_index < stack->allocated_pages * STACK_BYTES_PER_PAGE && stack->defined_and_merged[this_index / 4 / sizeof(packed_data_t)] & 1 << (this_index % (4 * sizeof(packed_data_t))) * 2;
		const int other_defined = other_index < other_stack->allocated_pages * STACK_BYTES_PER_PAGE && other_stack->defined_and_merged[other_index / 4 / sizeof(packed_data_t)] & 1 << (other_index % (4 * sizeof(packed_data_t))) * 2;
		const int this_merged = this_index < stack->allocated_pages * STACK_BYTES_PER_PAGE && stack->defined_and_merged[this_index / 4 / sizeof(packed_data_t)] & 2 << (this_index % (4 * sizeof(packed_data_t))) * 2;
		const int other_merged = other_index < other_stack->allocated_pages * STACK_BYTES_PER_PAGE && other_stack->defined_and_merged[other_index / 4 / sizeof(packed_data_t)] & 2 << (other_index % (4 * sizeof(packed_data_t))) * 2;
		int is_defined;

		if ((i & 1) == 0) {
			this_relative = (this_index / 16 / sizeof(packed_data_t)) < stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE && stack->relative[this_index / 16 / sizeof(packed_data_t)] & 1 << (this_index / 2) % (8 * sizeof(packed_data_t));
			other_relative = (other_index / 16 / sizeof(packed_data_t)) < other_stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE && other_stack->relative[other_index / 16 / sizeof(packed_data_t)] & 1 << (other_index / 2) % (8 * sizeof(packed_data_t));
		}

		is_defined = this_defined && other_defined && stack->data[this_index] == other_stack->data[other_index] && this_relative == other_relative;

		if (is_defined) {
			new_data[new_index] = stack->data[this_index];
			new_dnm[new_index / 4 / sizeof(packed_data_t)] |= 1 << (new_index % (4 * sizeof(packed_data_t))) * 2;
		}
		else if (this_defined || this_merged || other_defined || other_merged) {
			new_dnm[new_index / 4 / sizeof(packed_data_t)] &= ~(1 << (new_index % (4 * sizeof(packed_data_t))) * 2);
			new_dnm[new_index / 4 / sizeof(packed_data_t)] |= 2 << (new_index % (4 * sizeof(packed_data_t))) * 2;
		}
		else {
			new_dnm[new_index / 4 / sizeof(packed_data_t)] &= ~(3 << (new_index % (4 * sizeof(packed_data_t))) * 2);
		}

		if (i & 1) {
			if (this_relative && other_relative) {
				new_rel[new_index / 16 / sizeof(packed_data_t)] |= 1 << (new_index / 2) % (8 * sizeof(packed_data_t));
			}
			else {
				new_rel[new_index / 16 / sizeof(packed_data_t)] &= ~(1 << (new_index / 2) % (8 * sizeof(packed_data_t)));
			}
		}
	}

	clear_stack(stack);
	stack->allocated_pages = new_allocated_pages;
	stack->top = new_top;
	stack->data = new_data;
	stack->defined_and_merged = new_dnm;
	stack->relative = new_rel;

	return 0;
}

int changes_on_merging_stacks(const struct Stack *stack, const struct Stack *other_stack) {
	int this_bytes = stack->allocated_pages * STACK_BYTES_PER_PAGE - stack->top * 2;
	int other_bytes = other_stack->allocated_pages * STACK_BYTES_PER_PAGE - other_stack->top * 2;
	int required_bytes = (this_bytes < other_bytes)? other_bytes : this_bytes;

	int new_allocated_pages;
	int new_top;
	int this_relative;
	int other_relative;
	int is_defined = 0;
	int i;

	while (required_bytes > 0) {
		const int this_defined_or_merged = required_bytes + stack->top * 2 - 1 < stack->allocated_pages * STACK_BYTES_PER_PAGE && stack->defined_and_merged[(required_bytes + stack->top * 2 - 1) / 4 / sizeof(packed_data_t)] & 3 << ((required_bytes + stack->top * 2 - 1) % (4 * sizeof(packed_data_t))) * 2;

		if (this_defined_or_merged) {
			break;
		}
		else {
			const int other_defined_or_merged = required_bytes + other_stack->top * 2 - 1 < other_stack->allocated_pages * STACK_BYTES_PER_PAGE && other_stack->defined_and_merged[(required_bytes + other_stack->top * 2 - 1) / 4 / sizeof(packed_data_t)] & 3 << ((required_bytes + other_stack->top * 2 - 1) % (4 * sizeof(packed_data_t))) * 2;
			if (other_defined_or_merged) {
				break;
			}
			else {
				required_bytes--;
			}
		}
	}

	if (required_bytes & 1) {
		required_bytes++;
	}

	new_allocated_pages = (required_bytes + STACK_BYTES_PER_PAGE - 1) / STACK_BYTES_PER_PAGE;
	new_top = (new_allocated_pages - required_bytes) / 2;

	for (i = 0; i < required_bytes; i++) {
		const int this_index = stack->top * 2 + i;
		const int other_index = other_stack->top * 2 + i;
		const int this_defined = this_index < stack->allocated_pages * STACK_BYTES_PER_PAGE && stack->defined_and_merged[this_index / 4 / sizeof(packed_data_t)] & 1 << (this_index % (4 * sizeof(packed_data_t))) * 2;
		const int other_defined = other_index < other_stack->allocated_pages * STACK_BYTES_PER_PAGE && other_stack->defined_and_merged[other_index / 4 / sizeof(packed_data_t)] & 1 << (other_index % (4 * sizeof(packed_data_t))) * 2;
		const int this_merged = this_index < stack->allocated_pages * STACK_BYTES_PER_PAGE && stack->defined_and_merged[this_index / 4 / sizeof(packed_data_t)] & 2 << (this_index % (4 * sizeof(packed_data_t))) * 2;
		const int other_merged = other_index < other_stack->allocated_pages * STACK_BYTES_PER_PAGE && other_stack->defined_and_merged[other_index / 4 / sizeof(packed_data_t)] & 2 << (other_index % (4 * sizeof(packed_data_t))) * 2;

		if ((i & 1) == 0) {
			this_relative = (this_index / 16 / sizeof(packed_data_t)) < stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE && stack->relative[this_index / 16 / sizeof(packed_data_t)] & 1 << (this_index / 2) % (8 * sizeof(packed_data_t));
			other_relative = (other_index / 16 / sizeof(packed_data_t)) < other_stack->allocated_pages * STACK_BYTES_IN_REL_PER_PAGE && other_stack->relative[other_index / 16 / sizeof(packed_data_t)] & 1 << (other_index / 2) % (8 * sizeof(packed_data_t));
		}

		is_defined = this_defined && other_defined && stack->data[this_index] == other_stack->data[other_index] && this_relative == other_relative;

		if (this_defined && !is_defined) {
			return 1;
		}
	}

	return 0;
}

#ifdef DEBUG

#include <stdio.h>

#define STACK_UNKNOWN_COUNT_LIMIT 3

void print_stack(const struct Stack *stack) {
	const int end = stack->allocated_pages * STACK_BYTES_PER_PAGE / 2;
	int comma_required = 0;
	int unknown_count = 0;
	int i;

	fprintf(stderr, "Stack(");

	for (i = stack->top; i < end; i++) {
		const unsigned int definition = stack->defined_and_merged[(i * 2) / (4 * sizeof(packed_data_t))] >> ((i * 2) % (4 * sizeof(packed_data_t))) * 2;

		if ((definition & 5) == 0) {
			unknown_count++;
		}
		else {
			const int relative = stack->relative[i / (8 * sizeof(packed_data_t))] & 1 << i % (8 * sizeof(packed_data_t));

			if (comma_required) {
				fprintf(stderr, ", ");
			}
			comma_required = 0;

			if (unknown_count > STACK_UNKNOWN_COUNT_LIMIT) {
				fprintf(stderr, "?x%d", unknown_count);
				comma_required = 1;
			}
			else {
				int j;
				for (j = 0; j < unknown_count; j++) {
					if (comma_required) {
						fprintf(stderr, ", ");
					}

					fprintf(stderr, "?");
					comma_required = 1;
				}
			}
			unknown_count = 0;

			if (comma_required) {
				fprintf(stderr, ", ");
			}
			comma_required = 0;

			if (relative) {
				fprintf(stderr, "+");
			}

			if (definition & 4) {
				fprintf(stderr, "%02x", stack->data[i * 2 + 1]);
			}
			else {
				fprintf(stderr, "??");
			}

			if (definition & 1) {
				fprintf(stderr, "%02x", stack->data[i * 2]);
			}
			else {
				fprintf(stderr, "??");
			}

			comma_required = 1;
		}
	}

	fprintf(stderr, ")");
}

#endif /* DEBUG */
