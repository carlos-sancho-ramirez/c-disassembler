#ifndef _CODE_BLOCK_LIST_H_
#define _CODE_BLOCK_LIST_H_

#include "cblock.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(CodeBlock, block);
DECLARE_STRUCT_LIST_METHODS(CodeBlock, cblock, block, start);

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
