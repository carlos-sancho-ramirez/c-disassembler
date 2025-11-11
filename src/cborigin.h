#ifndef _CODE_BLOCK_ORIGIN_H_
#define _CODE_BLOCK_ORIGIN_H_

#include "register.h"
#include "gvwvmap.h"

/**
 * Denotes that this block is accessed directly by the OS. This is mainly saying that this block is the starting point of our executable.
 *
 * When the type is selected. value in instruction should be ignored.
 */
#define CBORIGIN_TYPE_OS 0

/**
 * Denotes that this block is accessed as a interruption handler.
 *
 * When the type is selected. value in instruction should be ignored.
 */
#define CBORIGIN_TYPE_INTERRUPTION 1

/**
 * Denotes that this block is accessed as the continuation of the previous one,
 * without any kind of jmp or call instruction.
 *
 * When this type is selected. value in instruction should be ignored.
 * Also we can assume that there is another existing code block whose end
 * matches the start of the block pointed by this origin.
 */
#define CBORIGIN_TYPE_CONTINUE 2

/**
 * Denotes that this block is accessed as the continuation of the previous one,
 * which is finishing with a call instruction (opcode E8, FF 1X and others),
 * far call or interruptions.
 *
 * When this type is selected. value in instruction should be ignored.
 * However, we also know that there is a valid behind count that can be queried
 * to check which instruction originated the call.
 *
 * Also we can assume that there is another existing code block whose end
 * matches the start of the block pointed by this origin.
 */
#define CBORIGIN_TYPE_CALL_RETURN 3

/**
 * Denotes that this block is accessed using a jmp or call instruction, or any of its variants, in any of our code instructions.
 *
 * When the type is selected. instruction field will point to the instruction
 * that originated this jump.
 *
 * Also, in this case, we can ensure that regs and variable maps are also
 * available and valid.
 */
#define CBORIGIN_TYPE_JUMP 4

/**
 * Provide information regarding how the linked block is reached.
 */
struct CodeBlockOrigin {
	/**
	 * Identifies the type, and holds some type-related properties.
	 * Depending on the value of these flags, other fields within this struct may be interpreted in a certain way.
	 */
	unsigned int flags;

	/**
	 * Instruction that performs the jump or call to this block.
	 *
	 * The value in this field is only valid if this origin has type JUMP.
	 * Call get_cborigin_type method to determine the correct type before relaying on this field.
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
int get_cborigin_type(const struct CodeBlockOrigin *origin);

/**
 * Checks if the cborigin is ready to be evaluated.
 *
 * This method is only defined if the origin type is CALL RETURN.
 */
int is_cborigin_ready_to_be_evaluated(const struct CodeBlockOrigin *origin);

int is_marked_as_never_reached(const struct CodeBlockOrigin *origin);

/**
 * Return the number of bytes that must be substrated to the start of this
 * block in order to find the call instruction that originated this origin.
 *
 * This method is only defined if the origin type is CALL RETURN.
 */
int get_cborigin_behind_count(const struct CodeBlockOrigin *origin);

/**
 * Set the OS type to the given code block origin.
 *
 * This will replace any previous set type.
 */
void set_os_type_in_cborigin(struct CodeBlockOrigin *origin);

/**
 * Set the INTERRUPTION type to the given code block origin.
 *
 * This will replace any previous set type.
 */
void set_interruption_type_in_cborigin(struct CodeBlockOrigin *origin);

/**
 * Set the CONTINUE type to the given code block origin. Leaving it not ready yet to be evaluated.
 *
 * This will replace any previous set type.
 */
void set_continue_type_in_cborigin(struct CodeBlockOrigin *origin);

/**
 * Set the CALL RETURN type and the given behind count to the given code block origin.
 *
 * This will replace any previous set type and count.
 */
void set_call_return_type_in_cborigin(struct CodeBlockOrigin *origin, unsigned int behind_count);

/**
 * Set the given origin as JUMP type and assigns the given instruction as well.
 *
 * This will replace any previous type and instruction set.
 */
void set_jump_type_in_cborigin(struct CodeBlockOrigin *origin, const char *instruction);

void set_cborigin_ready_to_be_evaluated(struct CodeBlockOrigin *origin);

/**
 * Mark a code block origin as never reached.
 *
 * This is only relevant if the origin type if CALL RETURN. And this should be
 * called when it is detected that the function called before this block will
 * never return, and then, it is not expected that this block will be executed
 * by returning from that function.
 */
void mark_cborigin_as_never_reached(struct CodeBlockOrigin *origin);

#endif /* _CODE_BLOCK_ORIGIN_H_ */
