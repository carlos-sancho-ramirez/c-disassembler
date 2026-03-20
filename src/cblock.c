#include "cblock.h"

#include <assert.h>

void initialize_cblock(struct CodeBlock *block, unsigned int relative_cs, unsigned int ip, const char *start, const char *end, const struct CodeBlockOriginList *origin_list) {
	assert(start != NULL && end != NULL && start < end && origin_list != NULL);

	block->relative_cs = relative_cs;
	block->ip = ip;
	block->start = start;
	block->end = end;
	block->origin_list = origin_list;
}

unsigned int get_cblock_relative_cs(const struct CodeBlock *block) {
	return block->relative_cs;
}

unsigned int get_cblock_ip(const struct CodeBlock *block) {
	return block->ip;
}

const char *get_cblock_start(const struct CodeBlock *block) {
	return block->start;
}

const char *get_cblock_end(const struct CodeBlock *block) {
	return block->end;
}

unsigned int get_cblock_size(const struct CodeBlock *block) {
	return block->end - block->start;
}

const struct CodeBlockOriginList *get_cblock_origin_list(const struct CodeBlock *block) {
	return block->origin_list;
}

int is_position_inside_cblock(const struct CodeBlock *block, const char *position) {
	return block->start <= position && position < block->end;
}

int has_cborigin_of_type_continue_in_cblock(const struct CodeBlock *block) {
	return index_of_cborigin_of_type_continue(block->origin_list) >= 0;
}

int has_cborigin_of_type_call_return_in_cblock(const struct CodeBlock *block, unsigned int behind_count) {
	return index_of_cborigin_of_type_call_return(block->origin_list, behind_count) >= 0;
}

int should_cblock_be_dumped(const struct CodeBlock *block) {
	return block->origin_list->origin_count > 0;
}

int should_dump_label_for_cblock(const struct CodeBlock *block) {
	const struct CodeBlockOriginList *origin_list = block->origin_list;
	const unsigned int origin_count = origin_list->origin_count;
	int index;
	for (index = 0; index < origin_count; index++) {
		const struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		const unsigned int origin_type = get_cborigin_type(origin);
		if (origin_type != CBORIGIN_TYPE_CONTINUE && origin_type != CBORIGIN_TYPE_CALL_RETURN) {
			return 1;
		}
	}

	return 0;
}

int index_of_block_with_start(const struct CodeBlock *blocks, unsigned int block_count, const char *start) {
	int first = 0;
	int last = block_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = blocks[index].start;
		if (this_start < start) {
			first = index + 1;
		}
		else if (this_start > start) {
			last = index;
		}
		else {
			return index;
		}
	}

	return -1;
}
