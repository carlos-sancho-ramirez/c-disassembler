#include "mcblock.h"
#include <assert.h>

#define CODE_BLOCK_FLAG_VALID_EVALUATION 1
#define CODE_BLOCK_FLAG_UNDER_EVALUATION 2

void initialize_mcblock(struct MutableCodeBlock *block, unsigned int relative_cs, unsigned int ip, const char *start) {
	block->relative_cs = relative_cs;
	block->ip = ip;
	block->start = start;
	block->end = start;
	block->flags = 0;
	initialize_cborigin_list(&block->origin_list);
}

unsigned int get_mcblock_relative_cs(const struct MutableCodeBlock *block) {
	return block->relative_cs;
}

unsigned int get_mcblock_ip(const struct MutableCodeBlock *block) {
	return block->ip;
}

const char *get_mcblock_start(const struct MutableCodeBlock *block) {
	return block->start;
}

int is_mcblock_end_known(const struct MutableCodeBlock *block) {
	return block->end > block->start;
}

const char *get_mcblock_end(const struct MutableCodeBlock *block) {
	return block->end;
}

const struct CodeBlockOriginList *get_mcblock_origin_list_const(const struct MutableCodeBlock *block) {
	return &block->origin_list;
}

struct CodeBlockOriginList *get_mcblock_origin_list(struct MutableCodeBlock *block) {
	return &block->origin_list;
}

unsigned int get_mcblock_size(const struct MutableCodeBlock *block) {
	return block->end - block->start;
}

void set_mcblock_end(struct MutableCodeBlock *block, const char *end) {
	assert(end > block->start);
	block->end = end;
}

int is_position_inside_mcblock(const struct MutableCodeBlock *block, const char *position) {
	return block->start <= position && position < block->end;
}

int has_cborigin_of_type_continue_in_mcblock(const struct MutableCodeBlock *block) {
	return index_of_cborigin_of_type_continue(&block->origin_list) >= 0;
}

int has_cborigin_of_type_call_return_in_mcblock(const struct MutableCodeBlock *block, unsigned int behind_count) {
	return index_of_cborigin_of_type_call_return(&block->origin_list, behind_count) >= 0;
}

void set_mcblock_size(struct MutableCodeBlock *block, unsigned int size) {
	assert(size > 0);
	block->end = block->start + size;
}

int mcblock_requires_evaluation(struct MutableCodeBlock *block) {
	return !(block->flags & CODE_BLOCK_FLAG_VALID_EVALUATION);
}

void mark_mcblock_as_being_evaluated(struct MutableCodeBlock *block) {
	block->flags |= CODE_BLOCK_FLAG_UNDER_EVALUATION | CODE_BLOCK_FLAG_VALID_EVALUATION;
}

void mark_mcblock_as_evaluated(struct MutableCodeBlock *block) {
	block->flags &= ~CODE_BLOCK_FLAG_UNDER_EVALUATION;
}

void invalidate_mcblock_check(struct MutableCodeBlock *block) {
	block->flags &= ~CODE_BLOCK_FLAG_VALID_EVALUATION;
}

int add_interruption_type_cborigin_in_mcblock(struct MutableCodeBlock *block, const struct Registers *regs, const struct GlobalVariableWordValueMap *var_values) {
	struct CodeBlockOriginList *origin_list = &block->origin_list;
	int error_code;
	int index = index_of_cborigin_with_type_interruption(origin_list);
	if (index < 0) {
		struct Registers accumulated_regs;
		struct Stack accumulated_stack;
		struct CodeBlockOrigin *new_origin;
		struct GlobalVariableWordValueMap accumulated_var_values;
		if (origin_list->origin_count) {
			accumulate_registers_from_cbolist(&accumulated_regs, origin_list);
			if ((error_code = accumulate_stack_from_cbolist(&accumulated_stack, origin_list)) ||
					(error_code = accumulate_gvwvmap_from_cbolist(&accumulated_var_values, origin_list))) {
				return error_code;
			}
		}

		new_origin = prepare_new_cborigin(origin_list);
		if ((error_code = initialize_cborigin_as_interruption(new_origin, regs, var_values)) ||
				(error_code = insert_cborigin(origin_list, new_origin))) {
			return error_code;
		}

		if (origin_list->origin_count > 1 && (changes_on_merging_registers(&accumulated_regs, regs) || changes_on_merging_gvwvmap(&accumulated_var_values, var_values))) {
			invalidate_mcblock_check(block);
		}
	}
	else {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		struct Registers *origin_regs = get_cborigin_registers(origin);
		struct GlobalVariableWordValueMap *origin_var_values = get_cborigin_var_values(origin);
		if (changes_on_merging_registers(origin_regs, regs) || changes_on_merging_gvwvmap(origin_var_values, var_values)) {
			merge_registers(origin_regs, regs);
			if ((error_code = merge_gvwvmap(origin_var_values, var_values))) {
				return error_code;
			}

			invalidate_mcblock_check(block);
		}
	}

	return 0;
}

int add_continue_type_cborigin_in_mcblock(struct MutableCodeBlock *block, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values) {
	int error_code;
	struct CodeBlockOriginList *origin_list;
	int index;

	origin_list = &block->origin_list;
	index = index_of_cborigin_of_type_continue(origin_list);
	if (index < 0) {
		struct CodeBlockOrigin *new_origin = prepare_new_cborigin(origin_list);
		struct Registers accumulated_regs;
		struct Stack accumulated_stack;
		struct GlobalVariableWordValueMap accumulated_var_values;

		if ((error_code = initialize_cborigin_as_continue(new_origin, regs, stack, var_values))) {
			return error_code;
		}

		if (origin_list->origin_count) {
			accumulate_registers_from_cbolist(&accumulated_regs, origin_list);
			if ((error_code = accumulate_stack_from_cbolist(&accumulated_stack, origin_list)) ||
					(error_code = accumulate_gvwvmap_from_cbolist(&accumulated_var_values, origin_list))) {
				return error_code;
			}
		}

		if ((error_code = insert_cborigin(origin_list, new_origin))) {
			return error_code;
		}

		if (origin_list->origin_count > 1 && (changes_on_merging_registers(&accumulated_regs, regs) || changes_on_merging_stacks(&accumulated_stack, stack) || changes_on_merging_gvwvmap(&accumulated_var_values, var_values))) {
			invalidate_mcblock_check(block);
		}
	}
	else {
		struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		struct Registers *origin_regs = get_cborigin_registers(origin);
		struct Stack *origin_stack = get_cborigin_stack(origin);
		struct GlobalVariableWordValueMap *origin_var_values = get_cborigin_var_values(origin);
		if (changes_on_merging_registers(origin_regs, regs) || changes_on_merging_stacks(origin_stack, stack) || changes_on_merging_gvwvmap(origin_var_values, var_values)) {
			merge_registers(origin_regs, regs);
			merge_stacks(origin_stack, stack);
			if ((error_code = merge_gvwvmap(origin_var_values, var_values))) {
				return error_code;
			}
			invalidate_mcblock_check(block);
		}
	}

	return 0;
}

int add_call_return_type_cborigin_in_mcblock(struct MutableCodeBlock *block, unsigned int behind_count, const struct Registers *regs, const struct Stack *stack, const struct GlobalVariableWordValueMap *var_values) {
	return add_call_return_type_cborigin(&block->origin_list, behind_count, regs, stack, var_values);
}

int should_mcblock_be_dumped(const struct MutableCodeBlock *block) {
	return block->origin_list.origin_count > 0;
}

int should_dump_label_for_mcblock(const struct MutableCodeBlock *block) {
	const struct CodeBlockOriginList *origin_list = &block->origin_list;
	const unsigned int origin_count = origin_list->origin_count;
	int index;
	for (index = 0; index < origin_count; index++) {
		const struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
		const unsigned int origin_type = get_cborigin_type(origin);
		if (origin_type != CBORIGIN_TYPE_CONTINUE && origin_type != CBORIGIN_TYPE_CALL_RETURN) {
			return 1;
		}
	}

	return 0;
}
