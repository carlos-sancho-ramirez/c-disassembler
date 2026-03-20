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

int has_cborigin_of_type_continue_in_cblock(const struct CodeBlock *block) {
	return index_of_cborigin_of_type_continue(block->origin_list) >= 0;
}

int has_cborigin_of_type_call_return_in_cblock(const struct CodeBlock *block, unsigned int behind_count) {
	return index_of_cborigin_of_type_call_return(block->origin_list, behind_count) >= 0;
}
