#ifndef _GLOBAL_VARIABLE_LIST_H_
#define _GLOBAL_VARIABLE_LIST_H_

#include "gvar.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(GlobalVariable, variable);
DECLARE_STRUCT_LIST_METHODS(GlobalVariable, global_variable, variable, start);

struct ReferenceList;
#include "register.h"
#include "sslist.h"

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

#endif /* _GLOBAL_VARIABLE_LIST_H_ */
