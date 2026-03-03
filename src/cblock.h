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
	 * This should always be equals or greater than start.
	 * This is initially set to match start. If so, it means that this block has not been read yet,
	 * and then we do not knwo for sure where it ends. Once the block is read, end will be placed
	 * to the first position outside the block, which may match with the start of another block or not.
	 */
	const char *end;

	unsigned int flags;

	/**
	 * List of origins found for this code block.
	 */
	struct CodeBlockOriginList origin_list;
};

/**
 * Initialize the CodeBlock structure with the given start.
 * This will intialize the block as empty, so end will match the given start as well.
 */
void initialize_cblock(struct CodeBlock *block, unsigned int relative_cs, unsigned int ip, const char *start);

unsigned int get_cblock_relative_cs(const struct CodeBlock *block);
unsigned int get_cblock_ip(const struct CodeBlock *block);
const char *get_cblock_start(const struct CodeBlock *block);
const char *get_cblock_end(const struct CodeBlock *block);
void set_cblock_end(struct CodeBlock *block, const char *end);

/**
 * Returns the difference between its start and end.
 * This method assumes that end is always greater or equals to start.
 */
unsigned int get_cblock_size(const struct CodeBlock *block);

/**
 * Whether the given block has the same position for start and end.
 */
int is_cblock_empty(const struct CodeBlock *block);

/**
 * Whether the given position is equals or greater than the block start position, and lower than the block end.
 */
int is_position_inside_cblock(const struct CodeBlock *block, const char *position);

/**
 * Set the block end with the result of adding the given size to the current block start.
 */
void set_cblock_size(struct CodeBlock *block, unsigned int size);

int cblock_requires_evaluation(struct CodeBlock *block);

/**
 * Return false if the block include a call return origin, and the registers are not yet set.
 *
 * If this is returning false, most probably there is another code block that must be evaluated before this one,
 * and that block will fulfill the missing registers.
 */
int cblock_ready_to_be_evaluated(const struct CodeBlock *block);
void mark_cblock_as_being_evaluated(struct CodeBlock *block);
void mark_cblock_as_evaluated(struct CodeBlock *block);
void invalidate_cblock_check(struct CodeBlock *block);

int add_interruption_type_cborigin_in_block(struct CodeBlock *block, const struct Registers *regs, const struct GlobalVariableWordValueMap *var_values);
int add_continue_type_cborigin_in_block(struct CodeBlock *block, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values);
int add_call_return_type_cborigin_in_block(struct CodeBlock *block, const struct Registers *regs, const struct Stack *stack, unsigned int behind_count);

/**
 * Whether it is valuable to be dumped.
 */
int should_be_dumped(const struct CodeBlock *block);

/**
 * Whether this block requires to print its label beofre dumping it.
 */
int should_dump_label_for_block(const struct CodeBlock *block);

#endif /* _CODE_BLOCK_H_ */
