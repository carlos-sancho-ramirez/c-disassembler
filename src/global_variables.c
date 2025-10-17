#include "global_variables.h"

DEFINE_STRUCT_LIST_METHODS(GlobalVariable, global_variable, variable, start, 8, 256)

#include "refs.h"

int add_global_variable_reference(
        struct GlobalVariableList *global_variable_list,
        struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list,
        struct Registers *regs,
        int segment_index,
        int result_address,
        const char *segment_start,
        const int value0,
        const char *opcode_reference) {
    if (is_segment_register_defined_and_relative(regs, segment_index)) {
        int error_code;
        unsigned int segment_value = get_segment_register(regs, segment_index);
        unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
        const char *target = segment_start + relative_address;
        const int var_index = index_of_global_variable_with_start(global_variable_list, target);
        struct GlobalVariable *var;
        if (var_index >= 0) {
            var = global_variable_list->sorted_variables[var_index];
        }
        else {
            var = prepare_new_global_variable(global_variable_list);
            var->start = target;
            var->relative_address = relative_address;
            if (value0 & 1) {
                var->end = target + 2;
                var->var_type = GLOBAL_VARIABLE_TYPE_WORD;
            }
            else {
                var->end = target + 1;
                var->var_type = GLOBAL_VARIABLE_TYPE_BYTE;
            }

            if ((error_code = insert_sorted_global_variable(global_variable_list, var))) {
                return error_code;
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

        if (index_of_reference_with_instruction(reference_list, opcode_reference) < 0) {
            struct Reference *new_ref = prepare_new_reference(reference_list);
            new_ref->instruction = opcode_reference;
            set_global_variable_reference_from_instruction_address(new_ref, var);
            if ((error_code = insert_sorted_reference(reference_list, new_ref))) {
                return error_code;
            }
        }
    }

    return 0;
}

int add_far_pointer_global_variable_reference(
        struct GlobalVariableList *global_variable_list,
        struct ReferenceList *reference_list,
        struct Registers *regs,
        int segment_index,
        int result_address,
        const char *segment_start,
        const char *opcode_reference) {
    if (is_segment_register_defined_and_relative(regs, segment_index)) {
        int error_code;
        unsigned int relative_address = (get_segment_register(regs, segment_index) * 16 + result_address) & 0xFFFF;
        const char *target = segment_start + relative_address;
        const int var_index = index_of_global_variable_with_start(global_variable_list, target);
        struct GlobalVariable *var;
        if (var_index >= 0) {
            var = global_variable_list->sorted_variables[var_index];
            if (var->var_type == GLOBAL_VARIABLE_TYPE_WORD) {
                var->end += 2;
                var->var_type = GLOBAL_VARIABLE_TYPE_FAR_POINTER;
            }
        }
        else {
            var = prepare_new_global_variable(global_variable_list);
            var->start = target;
            var->relative_address = relative_address;
            var->end = target + 4;
            var->var_type = GLOBAL_VARIABLE_TYPE_FAR_POINTER;

            if ((error_code = insert_sorted_global_variable(global_variable_list, var))) {
                return error_code;
            }
        }

        if (index_of_reference_with_instruction(reference_list, opcode_reference) < 0) {
            struct Reference *new_ref = prepare_new_reference(reference_list);
            new_ref->instruction = opcode_reference;
            set_global_variable_reference_from_instruction_address(new_ref, var);
            if ((error_code = insert_sorted_reference(reference_list, new_ref))) {
                return error_code;
            }
        }
    }

    return 0;
}
