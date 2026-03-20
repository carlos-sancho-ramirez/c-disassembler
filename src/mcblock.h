#ifndef _MUTABLE_CODE_BLOCK_H_
#define _MUTABLE_CODE_BLOCK_H_

#include "cbolist.h"
#include "cblock.h"

/**
 * Structure reflecting a piece of code whose instructions are always executed one after the other, except due to interruptions not explicitly called.
 */
struct MutableCodeBlock {
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
 * This will intialize the block with unknown end. End must be adjusted once we know where it is.
 */
void initialize_mcblock(struct MutableCodeBlock *block, unsigned int relative_cs, unsigned int ip, const char *start);

unsigned int get_mcblock_relative_cs(const struct MutableCodeBlock *block);
unsigned int get_mcblock_ip(const struct MutableCodeBlock *block);
const char *get_mcblock_start(const struct MutableCodeBlock *block);

/**
 * Whether the end of the block is known.
 *
 * Note that usually we identify a new block starting at certain position,
 * but we will not know where that block ends until we evaluate that block at least once.
 * Once traversed and found instructions like jmp or ret, we would be able to determine its possible end.
 *
 * This method should be called before calling get_mcblock_end and get_mcblock_size,
 * as their result is not determined if this method returns false.
 *
 * Note as well that the end can change later if we find other piece of the code jumping to a
 * position in the middle of our block. That would mean that we will need to split this block
 * in 2 pieces, and adjust the end of the block to the new start of the second half.
 * In any case, if the end needs to be adjusted, the new end will be always smaller
 * than the previous known one.
 */
int is_mcblock_end_known(const struct MutableCodeBlock *block);

/**
 * Returns the end of this code block.
 *
 * The end is always poiting to the first memory position outside the block, so the given end does not belong to the block.
 *
 * Do not call this method without calling is_mcblock_end_known first to ensure that the end is already known.
 * In case is_mcblock_end_known returns true, this method will return an end always greater than the current start, as no empty blocks are allowed.
 */
const char *get_mcblock_end(const struct MutableCodeBlock *block);
const struct CodeBlockOriginList *get_mcblock_origin_list_const(const struct MutableCodeBlock *block);
struct CodeBlockOriginList *get_mcblock_origin_list(struct MutableCodeBlock *block);

/**
 * Set the new end for this block.
 * The new end must be always greater than the current start.
 * Note that it is not allowed to have the end matching the start once the block has been evaluated at least once.
 */
void set_mcblock_end(struct MutableCodeBlock *block, const char *end);

/**
 * Returns the difference between its start and end.
 * Do not call this method without calling is_mcblock_end_known first to ensure that the end is already known.
 * In case is_mcblock_end_known returns true, this method will return a size greater than zero, as no empty blocks are allowed.
 */
unsigned int get_mcblock_size(const struct MutableCodeBlock *block);

/**
 * Whether the given position is equals or greater than the block start position, and lower than the block end.
 */
int is_position_inside_mcblock(const struct MutableCodeBlock *block, const char *position);

/**
 * Whether the given block has a code block origin of type continue registered in its origin list.
 */
int has_cborigin_of_type_continue_in_mcblock(const struct MutableCodeBlock *block);

/**
 * Whether the given block has a code block origin of type call return with the given behind_count registered in its origin list.
 */
int has_cborigin_of_type_call_return_in_mcblock(const struct MutableCodeBlock *block, unsigned int behind_count);

/**
 * Set the block end with the result of adding the given size to the current block start.
 * New size cannot be 0, as no empty blocks are allowed once the block has been evaluated at least one.
 */
void set_mcblock_size(struct MutableCodeBlock *block, unsigned int size);

int mcblock_requires_evaluation(struct MutableCodeBlock *block);

void mark_mcblock_as_being_evaluated(struct MutableCodeBlock *block);
void mark_mcblock_as_evaluated(struct MutableCodeBlock *block);
void invalidate_mcblock_check(struct MutableCodeBlock *block);

int add_interruption_type_cborigin_in_mcblock(struct MutableCodeBlock *block, const struct Registers *regs, const struct GlobalVariableWordValueMap *var_values);
int add_continue_type_cborigin_in_mcblock(struct MutableCodeBlock *block, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values);
int add_call_return_type_cborigin_in_mcblock(struct MutableCodeBlock *block, unsigned int behind_count, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values);

void copy_mcblock_to_cblock(struct CodeBlock *cblock, const struct MutableCodeBlock *mcblock);

/**
 * Whether it is valuable to be dumped.
 */
int should_mcblock_be_dumped(const struct MutableCodeBlock *block);

/**
 * Whether this block requires to print its label beofre dumping it.
 */
int should_dump_label_for_mcblock(const struct MutableCodeBlock *block);

#endif /* _MUTABLE_CODE_BLOCK_H_ */
