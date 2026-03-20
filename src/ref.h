#ifndef _REFERENCE_H_
#define _REFERENCE_H_

/**
 * Reference to a code block or a global variable.
 */
struct Reference {
	/**
	 * This can be an instance of GlobalVariable or and instance of CodeBlock, depending on the target_type flag.
	 */
	const void *target;

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

void initialize_ref(struct Reference *ref, const void *target, unsigned int flags, const char *instruction);

const char *get_ref_instruction(const struct Reference *ref);
struct GlobalVariable *get_gvar_from_ref_target(const struct Reference *ref);
struct CodeBlock *get_cblock_from_ref_target(const struct Reference *ref);
int is_ref_in_instruction_address(const struct Reference *ref);

#endif /* _REFERENCE_H_ */
