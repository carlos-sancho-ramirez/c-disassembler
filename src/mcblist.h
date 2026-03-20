#ifndef _MUTABLE_CODE_BLOCK_LIST_H_
#define _MUTABLE_CODE_BLOCK_LIST_H_

#include "mcblock.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(MutableCodeBlock, block);
DECLARE_STRUCT_LIST_METHODS(MutableCodeBlock, cblock, block, start);

/**
 * Returns a pointer to the code block containing the given position, or NULL if none matches.
 *
 * This method will first look for any block whose start matches the given position. If found, a pointer to that block will be returned.
 *
 * If none match, this method will then search for any block whose start is lower than the given
 * position and its end is greater, returning a pointer to it if found. Note that
 * this will only apply for block whose end is known.
 *
 * This method will return NULL if there are not blocks in the list, or none of them include the given position.
 */
struct MutableCodeBlock *get_cblock_containing_position(struct MutableCodeBlockList *list, const char *position);

/**
 * Returns a pointer to the code block whose start is equal or just after the given position.
 *
 * This will check for any code block whose start matches the given position. If it is found, a
 * pointer to that matching block will be returned.
 *
 * If none of the block in the list has an start matching the given position, this method will
 * return a pointer to the code block that has a start greater than the given position, but it
 * is the lowest among all the starts after the given position.
 *
 * If none of the blocks have an start greater than the given position, or this list is empty,
 * NULL will be returned.
 */
struct MutableCodeBlock *get_cblock_with_start_equals_or_after(const struct MutableCodeBlockList *list, const char *position);

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
int insert_cblock(struct MutableCodeBlockList *list, struct MutableCodeBlock *new_block);

/**
 * Return the sorted index of the given block within this list, or -1 if the block is not present.
 *
 * This method will not compare the given block with any potential block found in the list.
 * It will just pick the start of the block in order to check for it in the list, ignoring the rest of values.
 */
int index_of_cblock_in_list(const struct MutableCodeBlockList *list, const struct MutableCodeBlock *block);

/**
 * Return the sorted index of the block containing the origin JMP or CALL instruction, or -1 if none of the blocks in the list contains it.
 *
 * This method assumes that the given origin has type JUMP, and so, its instruction can be retrieved.
 */
int index_of_cblock_containing_origin_instruction(const struct MutableCodeBlockList *list, const struct CodeBlockOrigin *origin);

#ifdef DEBUG
void print_cblist(const struct MutableCodeBlockList *list);
#endif /* DEBUG */

#endif /* _MUTABLE_CODE_BLOCK_LIST_H_ */
