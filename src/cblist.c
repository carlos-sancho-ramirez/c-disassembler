#include "cblist.h"
#include "printd.h"

static void log_cblock_insertion(struct CodeBlock *block) {
	DEBUG_PRINT2("  Registering new code block at +%x:%x\n", get_cblock_relative_cs(block), get_cblock_ip(block));
}

DEFINE_STRUCT_LIST_INITIALIZE_METHOD(CodeBlock, cblock, block)
DEFINE_STRUCT_LIST_GET_UNSORTED_METHOD(CodeBlock, cblock, 64)
DEFINE_STRUCT_LIST_PREPARE_NEW_METHOD(CodeBlock, cblock, block, 8, 64)
DEFINE_STRUCT_LIST_CLEAR_METHOD(CodeBlock, cblock, block, 64)
DEFINE_STRUCT_LIST_INDEX_OF_WITH_METHOD(CodeBlock, cblock, block, start)

int insert_cblock(struct CodeBlockList *list, struct CodeBlock *new_block) {
	const char *new_block_start = get_cblock_start(new_block);
	int first = 0;
	int last = list->block_count;
	int i;
	log_cblock_insertion(new_block);
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = get_cblock_start(list->sorted_blocks[index]);
		if (this_start < new_block_start) {
			first = index + 1;
		}
		else if (this_start > new_block_start) {
			last = index;
		}
		else {
			return -1;
		}
	}

	if (last < list->block_count && get_cblock_start(list->sorted_blocks[last]) < get_cblock_end(new_block)) {
		return -1;
	}

	for (i = list->block_count; i > last; i--) {
		list->sorted_blocks[i] = list->sorted_blocks[i - 1];
	}

	list->sorted_blocks[last] = new_block;
	list->block_count++;

	return 0;
}

int index_of_cblock_containing_position(const struct CodeBlockList *list, const char *position) {
	int first = 0;
	int last = list->block_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = get_cblock_start(list->sorted_blocks[index]);
		if (this_start > position) {
			last = index;
		}
		else if (this_start == position) {
			return index;
		}
		else {
			const char *this_end = get_cblock_end(list->sorted_blocks[index]);
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

int index_of_cblock_in_list(const struct CodeBlockList *list, const struct CodeBlock *block) {
	return index_of_cblock_with_start(list, get_cblock_start(block));
}

int index_of_cblock_containing_origin_instruction(const struct CodeBlockList *list, const struct CodeBlockOrigin *origin) {
	return index_of_cblock_containing_position(list, get_cborigin_instruction(origin));
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
