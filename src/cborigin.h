#ifndef _CODE_BLOCK_ORIGIN_H_
#define _CODE_BLOCK_ORIGIN_H_

#include "register.h"
#include "gvwvmap.h"

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
 * Denotes that this block is accessed as the continuation of the previous one, without any kind of jmp or call instruction.
 *
 * When this type is selected. value in instruction should be ignored.
 * Also we can assume that there is another existing code block whose end matches the start of the block pointed by this origin.
 */
#define CODE_BLOCK_ORIGIN_TYPE_CONTINUE 2

/**
 * Denotes that this block is accessed as the continuation of the previous one, which is finishing with a call instruction
 * (opcode FF 1X) or any other whose instruction has exactly 2 bytes.
 *
 * When this type is selected. value in instruction should be ignored.
 * Also we can assume that there is another existing code block whose end matches the start of the block pointed by this origin.
 */
#define CODE_BLOCK_ORIGIN_TYPE_CALL_TWO_BEHIND 3

/**
 * Denotes that this block is accessed as the continuation of the previous one, which is finishing with a call instruction
 * (opcode E8) or any other whose instruction has exactly 3 bytes.
 *
 * When this type is selected. value in instruction should be ignored.
 * Also we can assume that there is another existing code block whose end matches the start of the block pointed by this origin.
 */
#define CODE_BLOCK_ORIGIN_TYPE_CALL_THREE_BEHIND 4

/**
 * Denotes that this block is accessed as the continuation of the previous one, which is finishing with a call instruction
 * (opcode FF XX) or any other whose instruction has exactly 4 bytes.
 *
 * When this type is selected. value in instruction should be ignored.
 * Also we can assume that there is another existing code block whose end matches the start of the block pointed by this origin.
 */
#define CODE_BLOCK_ORIGIN_TYPE_CALL_FOUR_BEHIND 5

/**
 * Denotes that this block is accessed using a jmp or call instruction, or any of its variants, in any of our code instructions.
 *
 * When the type is selected. instruction and regs fields are also available and valid.
 */
#define CODE_BLOCK_ORIGIN_TYPE_JUMP 6

/* These should be internal, but they are used by the its list... */
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_OS ((const char *) CODE_BLOCK_ORIGIN_TYPE_OS)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_INTERRUPTION ((const char *) CODE_BLOCK_ORIGIN_TYPE_INTERRUPTION)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CONTINUE ((const char *) CODE_BLOCK_ORIGIN_TYPE_CONTINUE)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND ((const char *) CODE_BLOCK_ORIGIN_TYPE_CALL_TWO_BEHIND)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND ((const char *) CODE_BLOCK_ORIGIN_TYPE_CALL_THREE_BEHIND)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND ((const char *) CODE_BLOCK_ORIGIN_TYPE_CALL_FOUR_BEHIND)

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
	 *
	 * In case cs is set to undefined, it means that values in regs and var_values are completelly unknown for now.
	 * This is a typical situation reached when the origin is the result of a call return, and we do not know yet what the function is returning.
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

/**
 * Returns a value different from 0 if the given instruction value is valid and does not conflict.
 */
int is_valid_instruction_value(const char *instruction);

#endif /* _CODE_BLOCK_ORIGIN_H_ */
