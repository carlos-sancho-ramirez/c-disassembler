#ifndef _CODE_BLOCKS_H_
#define _CODE_BLOCKS_H_

#include <stdlib.h>
#include "struct_list_macros.h"
#include "registers.h"
#include "interruption_table.h"

#define CODE_BLOCK_ORIGIN_INSTRUCTION_OS NULL
#define CODE_BLOCK_ORIGIN_BLOCK_OS NULL

#define CODE_BLOCK_ORIGIN_INSTRUCTION_INTERRUPTION NULL
#define CODE_BLOCK_ORIGIN_BLOCK_INTERRUPTION NULL

struct CodeBlockOrigin {
	const char *instruction;
	struct CodeBlock *block;
	struct Registers regs;
	struct InterruptionTable interruption_table;
};

DEFINE_STRUCT_LIST(CodeBlockOrigin, origin);
DECLARE_STRUCT_LIST_METHODS(CodeBlockOrigin, code_block_origin, origin, instruction);

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

	struct CodeBlockOriginList origin_list;
};

DEFINE_STRUCT_LIST(CodeBlock, block);
DECLARE_STRUCT_LIST_METHODS(CodeBlock, code_block, block, start);

#endif // _CODE_BLOCKS_H_