#include "cblock.h"
#include <assert.h>

#define CODE_BLOCK_FLAG_EVALUATED_AT_LEAST_ONCE 1
#define CODE_BLOCK_FLAG_VALID_EVALUATION 2
#define CODE_BLOCK_FLAG_UNDER_EVALUATION 4

int code_block_requires_evaluation(struct CodeBlock *block) {
	return !(block->flags & CODE_BLOCK_FLAG_VALID_EVALUATION);
}

int code_block_ready_to_be_evaluated(struct CodeBlock *block) {
	int index = index_of_code_block_origin_of_type_call_two_behind(&block->origin_list);
	if (index < 0) {
		index = index_of_code_block_origin_of_type_call_three_behind(&block->origin_list);
	}

	if (index < 0) {
		index = index_of_code_block_origin_of_type_call_four_behind(&block->origin_list);
	}

	if (index >= 0) {
		struct CodeBlockOrigin *origin = block->origin_list.sorted_origins[index];
		return is_register_cs_defined(&origin->regs);
	}
	else {
		return 1;
	}
}

void mark_code_block_as_being_evaluated(struct CodeBlock *block) {
	block->flags |= CODE_BLOCK_FLAG_UNDER_EVALUATION | CODE_BLOCK_FLAG_VALID_EVALUATION;
}

void mark_code_block_as_evaluated(struct CodeBlock *block) {
	block->flags |= CODE_BLOCK_FLAG_EVALUATED_AT_LEAST_ONCE;
	block->flags &= ~CODE_BLOCK_FLAG_UNDER_EVALUATION;
}

void invalidate_code_block_check(struct CodeBlock *block) {
	block->flags &= ~CODE_BLOCK_FLAG_VALID_EVALUATION;
}

int add_interruption_type_code_block_origin_in_block(struct CodeBlock *block, struct Registers *regs, struct GlobalVariableWordValueMap *var_values) {
	struct CodeBlockOriginList *origin_list = &block->origin_list;
	int error_code;
	int index = index_of_code_block_origin_with_instruction(origin_list, CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_INTERRUPTION);
	if (index < 0) {
		struct Registers accumulated_regs;
		struct CodeBlockOrigin *new_origin;
		struct GlobalVariableWordValueMap accumulated_var_values;
		if (origin_list->origin_count) {
			accumulate_registers_from_code_block_origin_list(&accumulated_regs, origin_list);
			accumulate_global_variable_word_values_from_code_block_origin_list(&accumulated_var_values, origin_list);
		}

		new_origin = prepare_new_code_block_origin(origin_list);
		new_origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_INTERRUPTION;
		copy_registers(&new_origin->regs, regs);
		copy_global_variable_word_values_map(&new_origin->var_values, var_values);
		if ((error_code = insert_sorted_code_block_origin(origin_list, new_origin))) {
			return error_code;
		}

		if (origin_list->origin_count > 1 && (changes_on_merging_registers(&accumulated_regs, regs) || changes_on_merging_global_variable_word_values_map(&accumulated_var_values, var_values))) {
			invalidate_code_block_check(block);
		}
	}
	else {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		if (changes_on_merging_registers(&origin->regs, regs)) {
			merge_registers(&origin->regs, regs);
			invalidate_code_block_check(block);
		}
	}

	return 0;
}

int add_call_two_behind_type_code_block_origin_in_block(struct CodeBlock *block) {
	return add_call_two_behind_type_code_block_origin(&block->origin_list);
}

int add_call_three_behind_type_code_block_origin_in_block(struct CodeBlock *block) {
	return add_call_three_behind_type_code_block_origin(&block->origin_list);
}

int add_call_four_behind_type_code_block_origin_in_block(struct CodeBlock *block) {
	return add_call_four_behind_type_code_block_origin(&block->origin_list);
}

int add_jump_type_code_block_origin_in_block(struct CodeBlock *block, const char *origin_instruction, struct Registers *regs, struct GlobalVariableWordValueMap *var_values) {
	int error_code;
	struct CodeBlockOriginList *origin_list;
	int index;

	assert(is_valid_instruction_value(origin_instruction));

	origin_list = &block->origin_list;
	index = index_of_code_block_origin_with_instruction(origin_list, origin_instruction);
	if (index < 0) {
		struct Registers accumulated_regs;
		struct GlobalVariableWordValueMap accumulated_var_values;
		struct CodeBlockOrigin *new_origin;
		if (origin_list->origin_count) {
			accumulate_registers_from_code_block_origin_list(&accumulated_regs, origin_list);
			accumulate_global_variable_word_values_from_code_block_origin_list(&accumulated_var_values, origin_list);
		}

		new_origin = prepare_new_code_block_origin(origin_list);
		new_origin->instruction = origin_instruction;
		copy_registers(&new_origin->regs, regs);
		copy_global_variable_word_values_map(&new_origin->var_values, var_values);
		if ((error_code = insert_sorted_code_block_origin(origin_list, new_origin))) {
			return error_code;
		}

		if (origin_list->origin_count > 1 && (changes_on_merging_registers(&accumulated_regs, regs) || changes_on_merging_global_variable_word_values_map(&accumulated_var_values, var_values))) {
			invalidate_code_block_check(block);
		}
	}
	else {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		if (changes_on_merging_registers(&origin->regs, regs)) {
			merge_registers(&origin->regs, regs);
			invalidate_code_block_check(block);
		}
	}

	return 0;
}
