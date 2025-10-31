#include "cbolist.h"
#include <assert.h>

DEFINE_STRUCT_LIST_METHODS(CodeBlockOrigin, code_block_origin, origin, instruction, 8, 4)

void accumulate_registers_from_code_block_origin_list(struct Registers *regs, struct CodeBlockOriginList *origin_list) {
	if (origin_list->origin_count) {
		int i;
		copy_registers(regs, &origin_list->sorted_origins[0]->regs);
		for (i = 0; i < origin_list->origin_count; i++) {
			merge_registers(regs, &origin_list->sorted_origins[i]->regs);
		}
	}
}

int accumulate_global_variable_word_values_from_code_block_origin_list(struct GlobalVariableWordValueMap *map, struct CodeBlockOriginList *origin_list) {
	if (origin_list->origin_count) {
		int error_code;
		int i;
		if ((error_code = copy_global_variable_word_values_map(map, &origin_list->sorted_origins[0]->var_values))) {
			return error_code;
		}

		for (i = 0; i < origin_list->origin_count; i++) {
			if ((error_code = merge_global_variable_word_values_map(map, &origin_list->sorted_origins[i]->var_values))) {
				return error_code;
			}
		}
	}

	return 0;
}

int index_of_code_block_origin_of_type_call_two_behind(struct CodeBlockOriginList *list) {
	return index_of_code_block_origin_with_instruction(list, CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND);
}

int index_of_code_block_origin_of_type_call_three_behind(struct CodeBlockOriginList *list) {
	return index_of_code_block_origin_with_instruction(list, CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND);
}

int index_of_code_block_origin_of_type_call_four_behind(struct CodeBlockOriginList *list) {
	return index_of_code_block_origin_with_instruction(list, CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND);
}

int add_call_two_behind_type_code_block_origin(struct CodeBlockOriginList *list) {
	if (index_of_code_block_origin_of_type_call_two_behind(list) < 0) {
		int error_code;
		struct CodeBlockOrigin *new_origin = prepare_new_code_block_origin(list);
		if (!new_origin) {
			return 1;
		}

		new_origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND;
		make_all_registers_undefined(&new_origin->regs);
		initialize_global_variable_word_value_map(&new_origin->var_values);
		if ((error_code = insert_sorted_code_block_origin(list, new_origin))) {
			return error_code;
		}
	}

	return 0;
}

int add_call_three_behind_type_code_block_origin(struct CodeBlockOriginList *list) {
	if (index_of_code_block_origin_of_type_call_three_behind(list) < 0) {
		int error_code;
		struct CodeBlockOrigin *new_origin = prepare_new_code_block_origin(list);
		if (!new_origin) {
			return 1;
		}

		new_origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND;
		make_all_registers_undefined(&new_origin->regs);
		initialize_global_variable_word_value_map(&new_origin->var_values);
		if ((error_code = insert_sorted_code_block_origin(list, new_origin))) {
			return error_code;
		}
	}

	return 0;
}

int add_call_four_behind_type_code_block_origin(struct CodeBlockOriginList *list) {
	if (index_of_code_block_origin_of_type_call_four_behind(list) < 0) {
		int error_code;
		struct CodeBlockOrigin *new_origin = prepare_new_code_block_origin(list);
		if (!new_origin) {
			return 1;
		}

		new_origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND;
		make_all_registers_undefined(&new_origin->regs);
		initialize_global_variable_word_value_map(&new_origin->var_values);
		if ((error_code = insert_sorted_code_block_origin(list, new_origin))) {
			return error_code;
		}
	}

	return 0;
}
