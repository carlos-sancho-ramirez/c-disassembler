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

int code_block_requires_evaluation(struct CodeBlock *block);

/**
 * Return false if the block include a call return origin, and the registers are not yet set.
 *
 * If this is returning false, most probably there is another code block that must be evaluated before this one,
 * and that block will fulfill the missing registers.
 */
int code_block_ready_to_be_evaluated(struct CodeBlock *block);
void mark_code_block_as_being_evaluated(struct CodeBlock *block);
void mark_code_block_as_evaluated(struct CodeBlock *block);
void invalidate_code_block_check(struct CodeBlock *block);

int add_interruption_type_code_block_origin_in_block(struct CodeBlock *block, struct Registers *regs, struct GlobalVariableWordValueMap *var_values);
int add_call_two_behind_type_code_block_origin_in_block(struct CodeBlock *block);
int add_call_three_behind_type_code_block_origin_in_block(struct CodeBlock *block);
int add_call_four_behind_type_code_block_origin_in_block(struct CodeBlock *block);
int add_jump_type_code_block_origin_in_block(struct CodeBlock *block, const char *origin_instruction, struct Registers *regs, struct GlobalVariableWordValueMap *var_values);

#endif /* _CODE_BLOCK_H_ */
