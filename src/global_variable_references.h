#ifndef _GLOBAL_VARIABLE_REFERENCES_H_
#define _GLOBAL_VARIABLE_REFERENCES_H_

#include "global_variables.h"

struct GlobalVariableReference {
	const char *opcode_reference;
	struct GlobalVariable *variable;
};

DEFINE_STRUCT_LIST(GlobalVariableReference, reference);
DECLARE_STRUCT_LIST_METHODS(GlobalVariableReference, global_variable_reference, reference, opcode_reference);

#endif // _GLOBAL_VARIABLE_REFERENCES_H_
