#ifndef _REFS_H_
#define _REFS_H_

struct Reference;
struct ReferenceList;

#include "gvar.h"
#include "cblock.h"

#define REFERENCE_FLAG_TARGET_TYPE_MASK 1
#define REFERENCE_FLAG_TARGET_IS_GLOBAL_VARIABLE 0
#define REFERENCE_FLAG_TARGET_IS_CODE_BLOCK 1

#define REFERENCE_FLAG_WHERE_IN_INSTRUCTION_MASK 2
#define REFERENCE_FLAG_IN_INSTRUCTION_ADDRESS 2
#define REFERENCE_FLAG_IN_INSTRUCTION_IMMEDIATE_VALUE 0

#define REFERENCE_FLAG_ACCESS_MASK 0x0C
#define REFERENCE_FLAG_ACCESS_READ 0x04
#define REFERENCE_FLAG_ACCESS_WRITE 0x08

/**
 * Reference to a global variable in the code.
 */
struct Reference {
	/**
	 * This can be an instance of GlobalVariable or and instance of CodeBlock, depending on the target_type flag.
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

/**
 * Returns the target of the given reference already casted as a GlobalVariable, or NULL if the target is not a GlobalVariable.
 */
struct GlobalVariable *get_global_variable_from_reference_target(struct Reference *ref);

/**
 * Returns the target of the given reference already casted as a CodeBlock, or NULL if the target is not a CodeBlock.
 */
struct CodeBlock *get_code_block_from_reference_target(struct Reference *ref);

/**
 * Sets the given global variable as the reference target, and mark it as located in the instruction address.
 * This will override any previous target in the reference.
 */
void set_global_variable_reference_from_instruction_address(struct Reference *ref, struct GlobalVariable *var);

/**
 * Sets the given global variable as the reference target, and mark it as located in the instruction immediate value.
 * This will override any previous target in the reference.
 */
void set_global_variable_reference_from_instruction_immediate_value(struct Reference *ref, struct GlobalVariable *var);

/**
 * Sets the given code block as the reference target, and mark it as located in the instruction immediate value.
 * This will override any previous target in the reference.
 */
void set_code_block_reference_from_instruction_immediate_value(struct Reference *ref, struct CodeBlock *block);

DEFINE_STRUCT_LIST(Reference, reference);
DECLARE_STRUCT_LIST_METHODS(Reference, reference, reference, instruction);

#endif
