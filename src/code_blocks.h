#ifndef _CODE_BLOCKS_H_
#define _CODE_BLOCKS_H_

#include <stdlib.h>
#include "struct_list_macros.h"

/**
 * Structure reflecting a piece of code that is always executed sequentially, except due to conditional jumps or interruptions.
 */
struct CodeBlock {
	unsigned int relative_cs;
	unsigned int ip;
	const char *start;

	/**
	 * This should always be equals or greater than start.
	 * This is initially set to match start. If so, it means that this block has not been read yet,
	 * and then we do not knwo for sure where it ends. Once the block is read, end will be placed
	 * to the first position outside the block, which may match with the start of another block or not.
	 */
	const char *end;
};

DEFINE_STRUCT_LIST(CodeBlock, block);
DECLARE_STRUCT_LIST_METHODS(CodeBlock, code_block, block, start);

#endif // _CODE_BLOCKS_H_