#ifndef _GLOBAL_VARIABLE_REFERENCES_H_
#define _GLOBAL_VARIABLE_REFERENCES_H_

#include "global_variables.h"

/**
 * Reference to a global variable in the code.
 */
struct GlobalVariableReference {
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
	struct GlobalVariable *value;
};

DEFINE_STRUCT_LIST(GlobalVariableReference, reference);
DECLARE_STRUCT_LIST_METHODS(GlobalVariableReference, global_variable_reference, reference, instruction);

#endif // _GLOBAL_VARIABLE_REFERENCES_H_
