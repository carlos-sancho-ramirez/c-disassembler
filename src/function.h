#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "cblist.h"

/**
 * Collect some blocks as a function unit.
 *
 * A function must have at least one block, and at least one block ending with
 * the instruction "ret", "retf" or "iret".
 */
struct Function {
	/**
	 * Flags for this function. Still nothing defined
	 */
	unsigned int flags;

	/**
	 * Array of block pointers for all blocks composing this function.
	 * Blocks are sorted by its start position.
	 */
	struct CodeBlock **blocks;

	/**
	 * Pointer to the first instruction of this function.
	 * This must be the first instruction of any of the blocks listed in the blocks array.
	 */
	const char *start;

	/**
	 * Number of blocks in the blocks array.
	 */
	unsigned int block_count;
};

#endif /* _FUNCTION_H_ */
