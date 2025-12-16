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
			const int origin_type = get_cborigin_type(origin);
			if (origin_index > 0) {
				fprintf(stderr, ", ");
			}

			if (origin_type == CBORIGIN_TYPE_OS) {
				fprintf(stderr, "OS");
			}
			else if (origin_type == CBORIGIN_TYPE_INTERRUPTION) {
				fprintf(stderr, "INT");
			}
			else if (origin_type == CBORIGIN_TYPE_CONTINUE) {
				fprintf(stderr, "CONT(");
				if (is_cborigin_ready_to_be_evaluated(origin)) {
					fprintf(stderr, "V)");
				}
				else {
					fprintf(stderr, "x)");
				}
			}
			else if (origin_type == CBORIGIN_TYPE_CALL_RETURN) {
				fprintf(stderr, "CR(%d,", get_cborigin_behind_count(origin));
				if (is_cborigin_ready_to_be_evaluated(origin)) {
					fprintf(stderr, "V)");
				}
				else {
					fprintf(stderr, "x)");
				}
			}
			else if (origin_type == CBORIGIN_TYPE_JUMP) {
				fprintf(stderr, "JMP");
			}
		}
		fprintf(stderr, ")");
	}

	fprintf(stderr, ")\n");
}

#endif /* DEBUG */
