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
	 * bit 2: Uses bp to refer to params or local variables in stack.
	 * bit 3: Starts with 'push bp' followed by 'mov bp,sp'
	 */
	unsigned int flags;

	/**
	 * Number of bytes that this function pops from the stack after its return.
	 */
	unsigned int return_size;

	/**
	 * Minimum number of word in the stack expected as arguments.
	 * This field is only relevant if bit 2 in flags is set.
	 *
	 * This field is worked out after evaluating the code searching for all references to [BP+X].
	 * The value in this field, if relevant, must be always equals or less than return_size / 2.
	 */
	unsigned int min_known_word_argument_count;

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

void initialize_func(struct Function *func);

int get_function_return_type(const struct Function *func);
int function_uses_bp(const struct Function *func);
int function_owns_bp(const struct Function *func);

const struct CodeBlock *get_start_block(const struct Function *func);

void set_function_return_type(struct Function *func, int return_type);
void set_function_uses_bp(struct Function *func, unsigned int min_known_word_argument_count);
void set_function_owns_bp(struct Function *func);

#ifdef DEBUG
void print_func(const struct Function *func);
#endif /* DEBUG */

#endif /* _FUNCTION_H_ */
