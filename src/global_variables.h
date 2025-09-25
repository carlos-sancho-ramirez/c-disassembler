#ifndef _GLOBAL_VARIABLES_H_
#define _GLOBAL_VARIABLES_H_

#include <stdlib.h>
#include "struct_list_macros.h"

#define GLOBAL_VARIABLE_TYPE_BYTE 1
#define GLOBAL_VARIABLE_TYPE_WORD 2
#define GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING 5

#define GLOBAL_VARIABLE_FLAG_READ 1
#define GLOBAL_VARIABLE_FLAG_WRITE 2

struct GlobalVariable {
	const char *start;
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
	unsigned int flags;
};

DEFINE_STRUCT_LIST(GlobalVariable, variable);
DECLARE_STRUCT_LIST_METHODS(GlobalVariable, global_variable, variable, start);

#endif // _GLOBAL_VARIABLES_H_