#ifndef _REF_H_
#define _REF_H_

/**
 * Reference to a global variable in the code.
 */
struct Reference {
	/**
	 * This can be an instance of GlobalVariable or and instance of MutableCodeBlock, depending on the target_type flag.
	 */
	void *target;

	/**
	 * Flags for this reference.
	 * Indicates things such as the type of target, where in the instruction is referenced or if this instruction is reading or writing from it.
	 */
	unsigned int flags;

	/**
	 * Points to the instruction where this reference appears.
	 */
	const char *instruction;
};

#include "gvar.h"
#include "mcblock.h"

/**
 * Initialize the given reference as global variable reference located in an instruction immediate value.
 *
 * This method will override any previous value in the given struct.
 */
void initialize_ref_as_gvar_instruction_immediate_value(struct Reference *ref, struct GlobalVariable *var, const char *instruction);

/**
 * Initialize the given reference as global variable reference located in an instruction address.
 *
 * This method will override any previous value in the given struct.
 */
void initialize_ref_as_gvar_instruction_address(struct Reference *ref, struct GlobalVariable *var, const char *instruction);

/**
 * Initialize the given reference as code block reference located in an instruction immediate value.
 *
 * This method will override any previous value in the given struct.
 */
void initialize_ref_as_cblock_instruction_immediate_value(struct Reference *ref, struct MutableCodeBlock *block, const char *instruction);

/**
 * Returns the instruction where this reference is located.
 */
const char *get_ref_instruction(struct Reference *ref);

/**
 * Returns the target of the given reference already casted as a GlobalVariable, or NULL if the target is not a GlobalVariable.
 */
struct GlobalVariable *get_gvar_from_ref_target(const struct Reference *ref);

/**
 * Returns the target of the given reference already casted as a MutableCodeBlock, or NULL if the target is not a MutableCodeBlock.
 */
struct MutableCodeBlock *get_cblock_from_ref_target(const struct Reference *ref);

/**
 * Whether this reference is located in the instruction address.
 */
int is_ref_in_instruction_address(const struct Reference *ref);

/**
 * Mark this reference as read access.
 *
 * This will not remove any write access set before. Note that some instructions reads and writes at the same time, like 'add' or 'xchg'.
 */
void set_gvar_ref_read_access(struct Reference *ref);

/**
 * Mark this reference as write access.
 *
 * This will not remove any read access set before. Note that some instructions reads and writes at the same time, like 'add' or 'xchg'.
 */
void set_gvar_ref_write_access(struct Reference *ref);

#endif /* _REF_H_ */
