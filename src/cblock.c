#include "cblock.h"
#include <assert.h>

#define CODE_BLOCK_FLAG_EVALUATED_AT_LEAST_ONCE 1
#define CODE_BLOCK_FLAG_VALID_EVALUATION 2
#define CODE_BLOCK_FLAG_UNDER_EVALUATION 4

int cblock_requires_evaluation(struct CodeBlock *block) {
	return !(block->flags & CODE_BLOCK_FLAG_VALID_EVALUATION);
}

int cblock_ready_to_be_evaluated(struct CodeBlock *block) {
	struct CodeBlockOriginList *list = &block->origin_list;
	int index = index_of_first_cborigin_of_type_call_return(&block->origin_list);
	struct CodeBlockOrigin *origin;
	if (index >= 0) {
		for (; index < list->origin_count && get_cborigin_type((origin = list->sorted_origins[index])) == CBORIGIN_TYPE_CALL_RETURN; index++) {
			if (!is_cborigin_ready_to_be_evaluated(origin)) {
				return 0;
			}
		}
	}

	return 1;
}

void mark_cblock_as_being_evaluated(struct CodeBlock *block) {
	block->flags |= CODE_BLOCK_FLAG_UNDER_EVALUATION | CODE_BLOCK_FLAG_VALID_EVALUATION;
}

void mark_cblock_as_evaluated(struct CodeBlock *block) {
	block->flags |= CODE_BLOCK_FLAG_EVALUATED_AT_LEAST_ONCE;
	block->flags &= ~CODE_BLOCK_FLAG_UNDER_EVALUATION;
}

void invalidate_cblock_check(struct CodeBlock *block) {
	block->flags &= ~CODE_BLOCK_FLAG_VALID_EVALUATION;
}

int add_interruption_type_cborigin_in_block(struct CodeBlock *block, struct Registers *regs, struct GlobalVariableWordValueMap *var_values) {
	struct CodeBlockOriginList *origin_list = &block->origin_list;
	int error_code;
	int index = index_of_cborigin_with_type_interruption(origin_list);
	if (index < 0) {
		struct Registers accumulated_regs;
		struct CodeBlockOrigin *new_origin;
		struct GlobalVariableWordValueMap accumulated_var_values;
		if (origin_list->origin_count) {
			accumulate_registers_from_cbolist(&accumulated_regs, origin_list);
			accumulate_gvwvmap_from_cbolist(&accumulated_var_values, origin_list);
		}

		new_origin = prepare_new_cborigin(origin_list);
		set_interruption_type_in_cborigin(new_origin);
		copy_registers(&new_origin->regs, regs);
		copy_gvwvmap(&new_origin->var_values, var_values);
		if ((error_code = insert_cborigin(origin_list, new_origin))) {
			return error_code;
		}

		if (origin_list->origin_count > 1 && (changes_on_merging_registers(&accumulated_regs, regs) || changes_on_merging_gvwvmap(&accumulated_var_values, var_values))) {
			invalidate_cblock_check(block);
		}
	}
	else {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		if (changes_on_merging_registers(&origin->regs, regs)) {
			merge_registers(&origin->regs, regs);
			invalidate_cblock_check(block);
		}
	}

	return 0;
}

int add_call_return_type_cborigin_in_block(struct CodeBlock *block, unsigned int behind_count) {
	return add_call_return_type_cborigin(&block->origin_list, behind_count);
}

int add_jump_type_cborigin_in_block(struct CodeBlock *block, const char *origin_instruction, const struct Registers *regs, const struct GlobalVariableWordValueMap *var_values) {
	int error_code;
	struct CodeBlockOriginList *origin_list;
	int index;

	origin_list = &block->origin_list;
	index = index_of_cborigin_with_instruction(origin_list, origin_instruction);
	if (index < 0) {
		struct Registers accumulated_regs;
		struct GlobalVariableWordValueMap accumulated_var_values;
		struct CodeBlockOrigin *new_origin;
		if (origin_list->origin_count) {
			accumulate_registers_from_cbolist(&accumulated_regs, origin_list);
			accumulate_gvwvmap_from_cbolist(&accumulated_var_values, origin_list);
		}

		new_origin = prepare_new_cborigin(origin_list);
		new_origin->instruction = origin_instruction;
		copy_registers(&new_origin->regs, regs);
		copy_gvwvmap(&new_origin->var_values, var_values);
		if ((error_code = insert_cborigin(origin_list, new_origin))) {
			return error_code;
		}

		if (origin_list->origin_count > 1 && (changes_on_merging_registers(&accumulated_regs, regs) || changes_on_merging_gvwvmap(&accumulated_var_values, var_values))) {
			invalidate_cblock_check(block);
		}
	}
	else {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		if (changes_on_merging_registers(&origin->regs, regs)) {
			merge_registers(&origin->regs, regs);
			invalidate_cblock_check(block);
		}
	}

	return 0;
}
