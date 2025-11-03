#ifndef _CODE_BLOCK_H_
#define _CODE_BLOCK_H_

#include "cbolist.h"

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

	unsigned int flags;
	struct CodeBlockOriginList origin_list;
};

int cblock_requires_evaluation(struct CodeBlock *block);

/**
 * Return false if the block include a call return origin, and the registers are not yet set.
 *
 * If this is returning false, most probably there is another code block that must be evaluated before this one,
 * and that block will fulfill the missing registers.
 */
int cblock_ready_to_be_evaluated(struct CodeBlock *block);
void mark_cblock_as_being_evaluated(struct CodeBlock *block);
void mark_cblock_as_evaluated(struct CodeBlock *block);
void invalidate_cblock_check(struct CodeBlock *block);

int add_interruption_type_cborigin_in_block(struct CodeBlock *block, struct Registers *regs, struct GlobalVariableWordValueMap *var_values);
int add_call_return_type_cborigin_in_block(struct CodeBlock *block, unsigned int behind_count);
int add_jump_type_cborigin_in_block(struct CodeBlock *block, const char *origin_instruction, const struct Registers *regs, const struct GlobalVariableWordValueMap *var_values);

#endif /* _CODE_BLOCK_H_ */
