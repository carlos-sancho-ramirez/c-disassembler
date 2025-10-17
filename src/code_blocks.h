#ifndef _CODE_BLOCKS_H_
#define _CODE_BLOCKS_H_

#include <stdlib.h>
#include "struct_list_macros.h"
#include "registers.h"
#include "interruption_table.h"
#include "global_variables.h"

/**
 * Denotes that this block is accessed directly by the OS. This is mainly saying that this block is the starting point of our executable.
 *
 * When the type is selected. value in instruction should be ignored.
 */
#define CODE_BLOCK_ORIGIN_TYPE_OS 0

/**
 * Denotes that this block is accessed as a interruption handler.
 *
 * When the type is selected. value in instruction should be ignored.
 */
#define CODE_BLOCK_ORIGIN_TYPE_INTERRUPTION 1

/**
 * Denotes that thsi block is accessed as the continuation of the previous one, without any kind of jmp or call instruction.
 *
 * When this type is selected. value in instruction should be ignored.
 * Also we can assume that there is another existing code block whose end matches the start of the block pointed by this origin.
 */
#define CODE_BLOCK_ORIGIN_TYPE_CONTINUE 2

/**
 * Denotes that this block is accessed using a jmp or call instruction, or any of its variants, in any of our code instructions.
 *
 * When the type is selected. instruction and regs fields are also available and valid.
 */
#define CODE_BLOCK_ORIGIN_TYPE_JUMP 3

/**
 * Provide information regarding how the linked block is reached.
 */
struct CodeBlockOrigin {
	/**
	 * Instruction that performs the jump or call to this block.
	 *
	 * The value in this field is only valid if this code block origin has type JUMP.
	 * Call get_code_block_origin_type method to determine the correct type before accessing this field.
	 */
	const char *instruction;

	/**
	 * State of the registers when the block is accessed by this origin.
	 */
	struct Registers regs;

	/**
	 * State of all known global variables when the block is accessed by this origin.
	 */
	struct GlobalVariableWordValueMap var_values;
};

/**
 * Return the type of code block origin. They can be any of the values represented by CODE_BLOCK_ORIGIN_TYPE_*.
 */
int get_code_block_origin_type(struct CodeBlockOrigin *origin);

/**
 * Set the OS type to the given code block origin.
 *
 * This will replace any previous set type.
 */
void set_os_type_in_code_block_origin(struct CodeBlockOrigin *origin);

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

	unsigned int flags;
	struct CodeBlockOriginList origin_list;
};

int code_block_requires_evaluation(struct CodeBlock *block);
void mark_code_block_as_being_evaluated(struct CodeBlock *block);
void mark_code_block_as_evaluated(struct CodeBlock *block);
void invalidate_code_block_check(struct CodeBlock *block);

DEFINE_STRUCT_LIST(CodeBlock, block);
DECLARE_STRUCT_LIST_METHODS(CodeBlock, code_block, block, start);

void accumulate_registers_from_code_block_origin_list(struct Registers *regs, struct CodeBlockOriginList *origin_list);
int add_interruption_type_code_block_origin(struct CodeBlock *block, struct Registers *regs, struct GlobalVariableWordValueMap *var_values);
int add_jump_type_code_block_origin(struct CodeBlock *block, const char *origin_instruction, struct Registers *regs, struct GlobalVariableWordValueMap *var_values);

int accumulate_global_variable_word_values_from_code_block_origin_list(struct GlobalVariableWordValueMap *map, struct CodeBlockOriginList *origin_list);
#endif // _CODE_BLOCKS_H_