#ifndef _REFS_H_
#define _REFS_H_

#include "global_variables.h"
#include "code_blocks.h"

/**
 * Reference to a global variable in the code.
 */
struct Reference {
	/**
	 * Points to the instruction where this reference appears.
	 */
	const char *instruction;

	/**
	 * Variable that must be used instead of a plain boring address.
	 * NULL if no address requires substitution.
	 */
	struct GlobalVariable *address;

	/**
	 * Variable that must be used instead of the numeric value in the instruction.
	 * NULL if no value requires substitution.
	 */
	struct GlobalVariable *variable_value;

	/**
	 * Code block that must be used instead of the numeric value in the instruction.
	 * NULL if no value requires substitution.
	 */
	struct CodeBlock *block_value;
};

DEFINE_STRUCT_LIST(Reference, reference);
DECLARE_STRUCT_LIST_METHODS(Reference, reference, reference, instruction);

#endif // _GLOBAL_VARIABLE_REFERENCES_H_
