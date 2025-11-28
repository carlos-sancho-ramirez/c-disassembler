#include "gvlist.h"
#include "slmacros.h"
#include "printd.h"

static void log_gvar_insertion(struct GlobalVariable *gvar) {
	DEBUG_PRINT1("  Registering new global variable at +%x\n", gvar->relative_address);
}

DEFINE_STRUCT_LIST_METHODS(GlobalVariable, gvar, variable, start, 8, 256)

int add_gvar_ref(
		struct GlobalVariableList *gvar_list,
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
		uint16_t write_value) {
	if (is_segment_register_defined_relative(regs, segment_index)) {
		int error_code;
		unsigned int segment_value = get_segment_register(regs, segment_index);
		unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
		const char *target = segment_start + relative_address;
		int index = index_of_gvar_with_start(gvar_list, target);
		struct GlobalVariable *var;
		struct Reference *var_ref;

		if (index >= 0) {
			var = gvar_list->sorted_variables[index];
		}
		else {
			var = prepare_new_gvar(gvar_list);
			var->start = target;
			var->relative_address = relative_address;
			if (value0 & 1) {
				var->end = target + 2;
				var->var_type = GVAR_TYPE_WORD;
			}
			else {
				var->end = target + 1;
				var->var_type = GVAR_TYPE_BYTE;
			}

			if ((error_code = insert_gvar(gvar_list, var))) {
				return error_code;
			}
		}

		if (write_access && write_value_defined) {
			if (write_value_defined_relative) {
				if ((error_code = put_gvar_in_gvwvmap_relative(var_values, target, write_value))) {
					return error_code;
				}
			}
			else {
				if ((error_code = put_gvar_in_gvwvmap(var_values, target, write_value))) {
					return error_code;
				}
			}
		}

		if (segment_value && segment_value != 0xFFF0) {
			const char *target_segment_start = segment_start + segment_value * 16;
			if (!contains_segment_start(segment_start_list, target_segment_start)) {
				if ((error_code = insert_segment_start(segment_start_list, target_segment_start))) {
					return error_code;
				}
			}
		}

		index = index_of_ref_with_instruction(reference_list, opcode_reference);
		if (index < 0) {
			var_ref = prepare_new_ref(reference_list);
			var_ref->instruction = opcode_reference;
			set_gvar_ref_from_instruction_address(var_ref, var);

			if ((error_code = insert_ref(reference_list, var_ref))) {
				return error_code;
			}
		}
		else {
			var_ref = reference_list->sorted_references[index];
		}

		if (read_access) {
			set_gvar_ref_read_access(var_ref);
		}

		if (write_access) {
			set_gvar_ref_write_access(var_ref);
		}
	}

	return 0;
}

int add_far_pointer_gvar_ref(
		struct GlobalVariableList *gvar_list,
		struct ReferenceList *ref_list,
		struct Registers *regs,
		int segment_index,
		int result_address,
		const char *segment_start,
		const char *opcode_reference) {
	if (is_segment_register_defined_relative(regs, segment_index)) {
		int error_code;
		unsigned int relative_address = (get_segment_register(regs, segment_index) * 16 + result_address) & 0xFFFF;
		const char *target = segment_start + relative_address;
		const int var_index = index_of_gvar_with_start(gvar_list, target);
		struct GlobalVariable *var;
		if (var_index >= 0) {
			var = gvar_list->sorted_variables[var_index];
			if (var->var_type == GVAR_TYPE_WORD) {
				var->end += 2;
				var->var_type = GVAR_TYPE_FAR_POINTER;
			}
		}
		else {
			var = prepare_new_gvar(gvar_list);
			var->start = target;
			var->relative_address = relative_address;
			var->end = target + 4;
			var->var_type = GVAR_TYPE_FAR_POINTER;

			if ((error_code = insert_gvar(gvar_list, var))) {
				return error_code;
			}
		}

		if (index_of_ref_with_instruction(ref_list, opcode_reference) < 0) {
			struct Reference *new_ref = prepare_new_ref(ref_list);
			new_ref->instruction = opcode_reference;
			set_gvar_ref_from_instruction_address(new_ref, var);
			if ((error_code = insert_ref(ref_list, new_ref))) {
				return error_code;
			}
		}
	}

	return 0;
}
