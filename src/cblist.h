#ifndef _CODE_BLOCK_LIST_H_
#define _CODE_BLOCK_LIST_H_

#include "cblock.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(CodeBlock, block);
DECLARE_STRUCT_LIST_METHODS(CodeBlock, cblock, block, start);

/**
 * Searches for a block containing the given position.
 *
 * This method will first look for any block whose start matches the given position. If found, the index of that block will be returned.
 *
 * If none match, this method will then search for any block whose start is lower than the given
 * position and its end is greater, returning its index if found. Note that
 * this will only apply for block whose end is known.
 *
 * This will return -1 if there are not blocks in the list, or none of them include the given position.
 */
int index_of_cblock_containing_position(const struct CodeBlockList *list, const char *position);

/**
 * Returns a pointer to the code block containing the given position, or NULL if none matches.
 */
struct CodeBlock *get_cblock_containing_position(struct CodeBlockList *list, const char *position);

/**
 * Inserts a new block into the list.
 *
 * For properly memory management, the given block pointer must be the same returned by
 * prepare_new_block method in its last call.
 *
 * This method will check the start and end properties in the given block and will evaluate
 * if the given block can be inserted without overlapping any existing block in this list.
 * In case the given block is overlapping any of the blocks already included in the list,
 * this method will return -1.
 *
 * If the given block is valid and not overlapping any of the existing blocks in the list,
 * this method will insert the block in the list in its suitable position. As result,
 * the block_count property will be increased by 1, and the sorted_blocks will be updated
 * accordingly.
 */
int insert_cblock(struct CodeBlockList *list, struct CodeBlock *new_block);

/**
 * Return the sorted index of the given block within this list, or -1 if the block is not present.
 *
 * This method will not compare the given block with any potential block found in the list.
 * It will just pick the start of the block in order to check for it in the list, ignoring the rest of values.
 */
int index_of_cblock_in_list(const struct CodeBlockList *list, const struct CodeBlock *block);

/**
 * Return the sorted index of the block containing the origin JMP or CALL instruction, or -1 if none of the blocks in the list contains it.
 *
 * This method assumes that the given origin has type JUMP, and so, its instruction can be retrieved.
 */
int index_of_cblock_containing_origin_instruction(const struct CodeBlockList *list, const struct CodeBlockOrigin *origin);

#ifdef DEBUG
void print_cblist(const struct CodeBlockList *list);
#endif /* DEBUG */

#endif /* _CODE_BLOCK_LIST_H_ */
