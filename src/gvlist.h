#ifndef _GLOBAL_VARIABLE_LIST_H_
#define _GLOBAL_VARIABLE_LIST_H_

#include "gvar.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(GlobalVariable, variable);
DECLARE_STRUCT_LIST_METHODS(GlobalVariable, gvar, variable, start);

#include "register.h"
#include "sslist.h"
#include "reflist.h"

/**
 * Inserts a new global variable reference.
 *
 * This method will first check if the global variable is already registered and will register it if not.
 * Also, in case of being a write access. This method will update the given variable map with the new value.
 *
 * Note that, in case write_access is 0, write_value, write_value_defined and write_value_defined_relative will be ignored.
 */
int add_gvar_ref(
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list,
		struct Registers *regs,
		struct GlobalVariableWordValueMap *var_values,
		int segment_index,
		int result_address,
		const char *segment_start,
		const int value0,
		const char *opcode_reference,
		const int read_access,
		const int write_access,
		const int write_value_defined,
		const int write_value_defined_relative,
		uint16_t write_value);

int add_far_pointer_gvar_ref(
		struct GlobalVariableList *global_variable_list,
		struct ReferenceList *reference_list,
		struct Registers *regs,
		int segment_index,
		int result_address,
		const char *segment_start,
		const char *opcode_reference,
		const int read_access);

#endif /* _GLOBAL_VARIABLE_LIST_H_ */
