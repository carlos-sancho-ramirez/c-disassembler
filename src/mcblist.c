#include "mcblist.h"
#include "printd.h"

static void log_cblock_insertion(struct MutableCodeBlock *block) {
	DEBUG_PRINT2("  Registering new code block at +%x:%x\n", get_mcblock_relative_cs(block), get_mcblock_ip(block));
}

DEFINE_STRUCT_LIST_INITIALIZE_METHOD(MutableCodeBlock, cblock, block)
DEFINE_STRUCT_LIST_GET_UNSORTED_METHOD(MutableCodeBlock, cblock, 64)
DEFINE_STRUCT_LIST_PREPARE_NEW_METHOD(MutableCodeBlock, cblock, block, 8, 64)
DEFINE_STRUCT_LIST_CLEAR_METHOD(MutableCodeBlock, cblock, block, 64)
DEFINE_STRUCT_LIST_INDEX_OF_WITH_METHOD(MutableCodeBlock, cblock, block, start)

int insert_cblock(struct MutableCodeBlockList *list, struct MutableCodeBlock *new_block) {
	const char *new_block_start = get_mcblock_start(new_block);
	int first = 0;
	int last = list->block_count;
	int i;
	log_cblock_insertion(new_block);
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = get_mcblock_start(list->sorted_blocks[index]);
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

	if (last < list->block_count && get_mcblock_start(list->sorted_blocks[last]) < get_mcblock_end(new_block)) {
		return -1;
	}

	for (i = list->block_count; i > last; i--) {
		list->sorted_blocks[i] = list->sorted_blocks[i - 1];
	}

	list->sorted_blocks[last] = new_block;
	list->block_count++;

	return 0;
}

static int index_of_cblock_containing_position(const struct MutableCodeBlockList *list, const char *position) {
	int first = 0;
	int last = list->block_count;
	while (last > first) {
		int index = (first + last) / 2;
		struct MutableCodeBlock *this_block = list->sorted_blocks[index];
		const char *this_start = get_mcblock_start(this_block);
		if (this_start > position) {
			last = index;
		}
		else if (this_start == position) {
			return index;
		}
		else {
			if (is_mcblock_end_known(this_block) && position < get_mcblock_end(this_block)) {
				return index;
			}
			else {
				first = index + 1;
			}
		}
	}

	return first - 1;
}

struct MutableCodeBlock *get_cblock_containing_position(struct MutableCodeBlockList *list, const char *position) {
	int index = index_of_cblock_containing_position(list, position);
	return (index < 0)? NULL : list->sorted_blocks[index];
}

static int index_of_cblock_with_start_equals_or_after(const struct MutableCodeBlockList *list, const char *position) {
	int first = 0;
	int last = list->block_count;

	while (last > first) {
		int index = (first + last) / 2;
		struct MutableCodeBlock *this_block = list->sorted_blocks[index];
		const char *this_start = get_mcblock_start(this_block);
		if (this_start > position) {
			last = index;
		}
		else if (this_start == position) {
			return index;
		}
		else {
			first = index + 1;
		}
	}

	return (last < list->block_count)? last : -1;
}

struct MutableCodeBlock *get_cblock_with_start_equals_or_after(const struct MutableCodeBlockList *list, const char *position) {
	int index = index_of_cblock_with_start_equals_or_after(list, position);
	return (index < 0)? NULL : list->sorted_blocks[index];
}

int index_of_cblock_in_list(const struct MutableCodeBlockList *list, const struct MutableCodeBlock *block) {
	return index_of_cblock_with_start(list, get_mcblock_start(block));
}

int index_of_cblock_containing_origin_instruction(const struct MutableCodeBlockList *list, const struct CodeBlockOrigin *origin) {
	return index_of_cblock_containing_position(list, get_cborigin_instruction(origin));
}

#ifdef DEBUG

#include <stdio.h>

void print_cblist(const struct MutableCodeBlockList *list) {
	int i;
	fprintf(stderr, "CodeBlockList(");
	for (i = 0; i < list->block_count; i++) {
		const struct MutableCodeBlock *block = list->sorted_blocks[i];
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
