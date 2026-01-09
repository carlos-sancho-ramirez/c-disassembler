#include "function.h"

#define FUNC_RETURN_TYPE_MASK 3
#define FUNC_FLAG_USES_BP 4
#define FUNC_FLAG_OWNS_BP 8

#include <assert.h>

static const packed_data_t *get_const_included_block_start(const struct Function *func) {
	if (func->block_count <= sizeof(packed_data_t *) * 8) {
		return (const packed_data_t *) &func->included_block_start;
	}
	else {
		return func->included_block_start;
	}
}

packed_data_t *get_included_block_start(struct Function *func) {
	if (func->block_count <= sizeof(packed_data_t *) * 8) {
		return (packed_data_t *) &func->included_block_start;
	}
	else {
		return func->included_block_start;
	}
}

int initialize_func(struct Function *func, unsigned int block_count) {
	assert(block_count > 0);
	func->flags = 0;
	func->block_count = block_count;

	if (block_count <= sizeof(packed_data_t *) * 8) {
		func->included_block_start = NULL;
	}
	else {
		func->included_block_start = allocate_bitset(block_count);
		if (!func->included_block_start) {
			return 1;
		}
	}

	return 0;
}

void free_func_content(struct Function *func) {
	if (func->block_count > sizeof(packed_data_t *) * 8) {
		free(func->included_block_start);
	}
}

int get_function_return_type(const struct Function *func) {
	return func->flags & FUNC_RETURN_TYPE_MASK;
}

int function_uses_bp(const struct Function *func) {
	return func->flags & FUNC_FLAG_USES_BP;
}

int function_owns_bp(const struct Function *func) {
	return func->flags & FUNC_FLAG_OWNS_BP;
}

unsigned int get_starting_block_count(const struct Function *func) {
	const packed_data_t *bitset = get_const_included_block_start(func);
	return count_set_bits_in_bitset(bitset, func->block_count);
}

const struct CodeBlock *get_starting_block(const struct Function *func, unsigned int index) {
	const unsigned int block_count = func->block_count;
	const packed_data_t *bitset = get_const_included_block_start(func);
	int included_block_index;
	int count = 0;

	for (included_block_index = 0; included_block_index < block_count; included_block_index++) {
		if (get_bitset_value(bitset, included_block_index)) {
			if (count++ == index) {
				return func->blocks[included_block_index];
			}
		}
	}

	return NULL;
}

void set_function_return_type(struct Function *func, int return_type) {
	assert((return_type & FUNC_RETURN_TYPE_MASK) == return_type);
	func->flags &= ~FUNC_RETURN_TYPE_MASK;
	func->flags |= return_type;
}

void set_function_uses_bp(struct Function *func, unsigned int min_known_word_argument_count) {
	func->flags |= FUNC_FLAG_USES_BP;
	func->min_known_word_argument_count = min_known_word_argument_count;
}

void set_function_owns_bp(struct Function *func) {
	func->flags |= FUNC_FLAG_OWNS_BP;
}

#ifdef DEBUG

#include <stdio.h>

void print_func(const struct Function *func) {
	const unsigned int block_count = func->block_count;
	const packed_data_t *included_block_start = get_const_included_block_start(func);
	unsigned int block_index;

	fprintf(stderr, "+%X{", func->blocks[0]->relative_cs);
	for (block_index = 0; block_index < block_count; block_index++) {
		if (block_index > 0) {
			fprintf(stderr, ", ");
		}

		if (get_bitset_value(included_block_start, block_index)) {
			fprintf(stderr, "*");
		}

		fprintf(stderr, "%X", func->blocks[block_index]->ip);
	}

	fprintf(stderr, "}");

	if (function_uses_bp(func)) {
		if (function_owns_bp(func)) {
			fprintf(stderr, " OWNS_BP(args=%u)", func->min_known_word_argument_count);
		}
		else {
			fprintf(stderr, " USES_BP(args=%u)", func->min_known_word_argument_count);
		}
	}
	else if (function_owns_bp(func)) {
		fprintf(stderr, " OWNS_BP");
	}

	if (get_function_return_type(func) == FUNC_RET_TYPE_NEAR) {
		if (func->return_size > 0) {
			fprintf(stderr, " NEAR(%d bytes)", func->return_size);
		}
		else {
			fprintf(stderr, " NEAR");
		}
	}
	else if (get_function_return_type(func) == FUNC_RET_TYPE_FAR) {
		fprintf(stderr, " FAR");
	}
	else if (get_function_return_type(func) == FUNC_RET_TYPE_INT) {
		fprintf(stderr, " INTER");
	}
}

#endif /* DEBUG */
