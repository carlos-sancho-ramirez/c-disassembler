#include "cblist.h"
#include "printd.h"

static void log_cblock_insertion(struct CodeBlock *block) {
	DEBUG_PRINT2("  Registering new code block at +%x:%x\n", block->relative_cs, block->ip);
}

DEFINE_STRUCT_LIST_INITIALIZE_METHOD(CodeBlock, cblock, block)
DEFINE_STRUCT_LIST_GET_UNSORTED_METHOD(CodeBlock, cblock, 64)
DEFINE_STRUCT_LIST_PREPARE_NEW_METHOD(CodeBlock, cblock, block, 8, 64)
DEFINE_STRUCT_LIST_CLEAR_METHOD(CodeBlock, cblock, block, 64)
DEFINE_STRUCT_LIST_INDEX_OF_WITH_METHOD(CodeBlock, cblock, block, start)
DEFINE_STRUCT_LIST_INSERT_METHOD(CodeBlock, cblock, block, start)

int index_of_cblock_containing_position(const struct CodeBlockList *list, const char *position) {
	int first = 0;
	int last = list->block_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->sorted_blocks[index]->start;
		if (this_start > position) {
			last = index;
		}
		else if (this_start == position) {
			return index;
		}
		else {
			const char *this_end = list->sorted_blocks[index]->end;
			if (this_end > position) {
				return index;
			}
			else {
				first = index + 1;
			}
		}
	}

	return first - 1;
}

#ifdef DEBUG

#include <stdio.h>

void print_cblist(const struct CodeBlockList *list) {
	int i;
	fprintf(stderr, "CodeBlockList(");
	for (i = 0; i < list->block_count; i++) {
		const struct CodeBlock *block = list->sorted_blocks[i];
		const struct CodeBlockOriginList *origin_list = &block->origin_list;
		int origin_index;
		if (i > 0) {
			fprintf(stderr, ", ");
		}

		fprintf(stderr, "+%X:%X-%X_%d(", block->relative_cs, block->ip, (int) (block->ip + (block->end - block->start)), block->flags);
		for (origin_index = 0; origin_index < origin_list->origin_count; origin_index++) {
			struct CodeBlockOrigin *origin = origin_list->sorted_origins[origin_index];
			if (origin_index > 0) {
				fprintf(stderr, ", ");
			}

			print_cborigin(origin);
		}
		fprintf(stderr, ")");
	}

	fprintf(stderr, ")\n");
}

#endif /* DEBUG */
