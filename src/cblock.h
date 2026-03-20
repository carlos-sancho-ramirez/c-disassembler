#ifndef _CODE_BLOCK_H_
#define _CODE_BLOCK_H_

#include "cbolist.h"

/**
 * Structure reflecting a piece of code whose instructions are always executed one after the other, except due to interruptions not explicitly called.
 */
struct CodeBlock {
	unsigned int relative_cs;
	unsigned int ip;
	const char *start;

	/**
	 * This should always be greater than start.
	 */
	const char *end;

	/**
	 * List of origins found for this code block.
	 */
	const struct CodeBlockOriginList *origin_list;
};

void initialize_cblock(struct CodeBlock *block, unsigned int relative_cs, unsigned int ip, const char *start, const char *end, const struct CodeBlockOriginList *origin_list);

unsigned int get_cblock_relative_cs(const struct CodeBlock *block);
unsigned int get_cblock_ip(const struct CodeBlock *block);
const char *get_cblock_start(const struct CodeBlock *block);
const char *get_cblock_end(const struct CodeBlock *block);
unsigned int get_cblock_size(const struct CodeBlock *block);
const struct CodeBlockOriginList *get_cblock_origin_list(const struct CodeBlock *block);

int has_cborigin_of_type_continue_in_cblock(const struct CodeBlock *block);
int has_cborigin_of_type_call_return_in_cblock(const struct CodeBlock *block, unsigned int behind_count);

#endif /* _CODE_BLOCK_H_ */
