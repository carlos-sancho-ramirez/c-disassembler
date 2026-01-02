#include "function.h"

#define FUNC_RETURN_TYPE_MASK 3
#define FUNC_FLAG_USES_BP 4
#define FUNC_FLAG_OWNS_BP 8

#include <assert.h>

void initialize_func(struct Function *func) {
	func->flags = 0;
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

const struct CodeBlock *get_start_block(const struct Function *func) {
	int i;

	for (i = 0; i < func->block_count; i++) {
		struct CodeBlock *block = func->blocks[i];
		if (block->start == func->start) {
			return block;
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
	const struct CodeBlock *start_block = get_start_block(func);
	unsigned int block_index;

	if (start_block) {
		fprintf(stderr, "+%X:%X(", start_block->relative_cs, start_block->ip);
	}
	else {
		fprintf(stderr, "?(");
	}

	for (block_index = 0; block_index < func->block_count; block_index++) {
		if (block_index > 0) {
			fprintf(stderr, ", ");
		}
		fprintf(stderr, "%X", func->blocks[block_index]->ip);
	}

	fprintf(stderr, ")");

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
