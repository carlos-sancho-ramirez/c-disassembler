#include "funclist.h"
#include "printd.h"

static void log_func_insertion(struct Function *func) {
	const struct CodeBlock *start_block = get_start_block(func);
	DEBUG_PRINT2("  Registering new function at +%x:%x\n", start_block->relative_cs, start_block->ip);
}

DEFINE_STRUCT_LIST_METHODS(Function, func, func, start, 8, 64)

int index_of_func_containing_block_start(struct FunctionList *list, const char *start) {
	int func_index;
	for (func_index = 0; func_index < list->func_count; func_index++) {
		struct Function *func = list->sorted_funcs[func_index];
		struct CodeBlock **func_blocks = func->blocks;
		int block_index;

		for (block_index = 0; block_index < func->block_count; block_index++) {
			if (func_blocks[block_index]->start == start) {
				return func_index;
			}
		}
	}

	return -1;
}

#ifdef DEBUG

#include <stdio.h>

void print_funclist(const struct FunctionList *list) {
	int i;
	fprintf(stderr, "FunctionList(\n ");
	for (i = 0; i < list->func_count; i++) {
		const struct Function *func = list->sorted_funcs[i];
		const struct CodeBlock *start_block = get_start_block(func);
		unsigned int block_index;

		print_func(func);
		if (i + 1 == list->func_count) {
			fprintf(stderr, ")");
		}
		fprintf(stderr, "\n ");
	}
}

#endif /* DEBUG */
