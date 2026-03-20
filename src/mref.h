#ifndef _MUTABLE_REFERENCE_H_
#define _MUTABLE_REFERENCE_H_

/**
 * MutableReference to a global variable in the code.
 */
struct MutableReference {
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
#include "ref.h"

/**
 * Initialize the given reference as global variable reference located in an instruction immediate value.
 *
 * This method will override any previous value in the given struct.
 */
void initialize_mref_as_gvar_instruction_immediate_value(struct MutableReference *ref, struct GlobalVariable *var, const char *instruction);

/**
 * Initialize the given reference as global variable reference located in an instruction address.
 *
 * This method will override any previous value in the given struct.
 */
void initialize_mref_as_gvar_instruction_address(struct MutableReference *ref, struct GlobalVariable *var, const char *instruction);

/**
 * Initialize the given reference as code block reference located in an instruction immediate value.
 *
 * This method will override any previous value in the given struct.
 */
void initialize_mref_as_cblock_instruction_immediate_value(struct MutableReference *ref, struct MutableCodeBlock *block, const char *instruction);

/**
 * Returns the instruction where this reference is located.
 */
const char *get_mref_instruction(struct MutableReference *ref);

/**
 * Returns the target of the given reference already casted as a GlobalVariable, or NULL if the target is not a GlobalVariable.
 */
struct GlobalVariable *get_gvar_from_mref_target(const struct MutableReference *ref);

/**
 * Returns the target of the given reference already casted as a MutableCodeBlock, or NULL if the target is not a MutableCodeBlock.
 */
struct MutableCodeBlock *get_mcblock_from_mref_target(const struct MutableReference *ref);

/**
 * Whether this reference is located in the instruction address.
 */
int is_mref_in_instruction_address(const struct MutableReference *ref);

/**
 * Mark this reference as read access.
 *
 * This will not remove any write access set before. Note that some instructions reads and writes at the same time, like 'add' or 'xchg'.
 */
void set_gvar_mref_read_access(struct MutableReference *ref);

/**
 * Mark this reference as write access.
 *
 * This will not remove any read access set before. Note that some instructions reads and writes at the same time, like 'add' or 'xchg'.
 */
void set_gvar_mref_write_access(struct MutableReference *ref);

int copy_mref_to_ref(struct Reference *target, const struct MutableReference *source, const struct CodeBlock *blocks, unsigned int block_count);

#endif /* _MUTABLE_REFERENCE_H_ */
