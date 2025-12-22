#include "cblist.h"
#include "printd.h"

static void log_cblock_insertion(struct CodeBlock *block) {
	DEBUG_PRINT2("  Registering new code block at +%x:%x\n", block->relative_cs, block->ip);
}

DEFINE_STRUCT_LIST_METHODS(CodeBlock, cblock, block, start, 8, 64)

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
