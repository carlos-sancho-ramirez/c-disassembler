#ifndef _GLOBAL_VARIABLES_H_
#define _GLOBAL_VARIABLES_H_

#include <stdlib.h>
#include "struct_list_macros.h"

#define GLOBAL_VARIABLE_TYPE_BYTE 1
#define GLOBAL_VARIABLE_TYPE_WORD 2
#define GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING 5
#define GLOBAL_VARIABLE_TYPE_FAR_POINTER 9

struct GlobalVariable {
	const char *start;
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
};

DEFINE_STRUCT_LIST(GlobalVariable, variable);
DECLARE_STRUCT_LIST_METHODS(GlobalVariable, global_variable, variable, start);

#include "refs.h"
#include "registers.h"
#include "segments.h"

int add_global_variable_reference(
        struct GlobalVariableList *global_variable_list,
        struct SegmentStartList *segment_start_list,
	struct ReferenceList *reference_list,
        struct Registers *regs,
        int segment_index,
        int result_address,
        const char *segment_start,
        const int value0,
        const char *opcode_reference);

int add_far_pointer_global_variable_reference(
        struct GlobalVariableList *global_variable_list,
        struct ReferenceList *reference_list,
        struct Registers *regs,
        int segment_index,
        int result_address,
        const char *segment_start,
        const char *opcode_reference);

#endif // _GLOBAL_VARIABLES_H_