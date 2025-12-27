#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "cblist.h"

#define FUNC_RET_TYPE_NEAR 1
#define FUNC_RET_TYPE_FAR 2
#define FUNC_RET_TYPE_INT 3

/**
 * Collect some blocks as a function unit.
 *
 * A collection of blocks are considered a function if, and only if, meet all the following conditions:
 * - It must contain at least one block.
 * - It must finish with one or more blocks ending with instruction "ret",
 *   "retf" or "iret", if there is more than one endign blocks, all of them
 *   must finish with the same instruction and number in case of "ret".
 *   (E.g. if one finishes with "ret 0x0006", all the rest must finish with
 *   "ret" and returning 6).
 * - All blocks must be in the same code segment. which means that all of them
 *   must have the same relative CS.
 * - Blocks cannot be shared with other functions. So, all blocks that belongs
 *   to a function cannot belong to other.
 *
 * On the other side, for simplicity, we will consider the following, but it
 * can change in the future if required.
 * - There is only one starting block for each function.
 */
struct Function {
	/**
	 * Flags for this function.
	 * bits 1-0: Type of return type. This must be near, far or int.
	 */
	unsigned int flags;

	/**
	 * Number of bytes that this function pops from the stack after its return.
	 */
	unsigned int return_size;

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
