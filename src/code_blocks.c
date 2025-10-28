#include "code_blocks.h"
#include "global_variables.h"
#include <assert.h>

#define CODE_BLOCK_FLAG_EVALUATED_AT_LEAST_ONCE 1
#define CODE_BLOCK_FLAG_VALID_EVALUATION 2
#define CODE_BLOCK_FLAG_UNDER_EVALUATION 4

#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_OS ((const char *) CODE_BLOCK_ORIGIN_TYPE_OS)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_INTERRUPTION ((const char *) CODE_BLOCK_ORIGIN_TYPE_INTERRUPTION)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CONTINUE ((const char *) CODE_BLOCK_ORIGIN_TYPE_CONTINUE)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND ((const char *) CODE_BLOCK_ORIGIN_TYPE_CALL_TWO_BEHIND)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND ((const char *) CODE_BLOCK_ORIGIN_TYPE_CALL_THREE_BEHIND)
#define CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND ((const char *) CODE_BLOCK_ORIGIN_TYPE_CALL_FOUR_BEHIND)

int code_block_requires_evaluation(struct CodeBlock *block) {
    return !(block->flags & CODE_BLOCK_FLAG_VALID_EVALUATION);
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

int get_code_block_origin_type(struct CodeBlockOrigin *origin) {
    const char *instruction = origin->instruction;
    return (instruction == CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_OS)? CODE_BLOCK_ORIGIN_TYPE_OS :
            (instruction == CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_INTERRUPTION)? CODE_BLOCK_ORIGIN_TYPE_INTERRUPTION :
            (instruction == CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CONTINUE)? CODE_BLOCK_ORIGIN_TYPE_CONTINUE :
            (instruction == CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND)? CODE_BLOCK_ORIGIN_TYPE_CALL_TWO_BEHIND :
            (instruction == CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND)? CODE_BLOCK_ORIGIN_TYPE_CALL_THREE_BEHIND :
            (instruction == CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND)? CODE_BLOCK_ORIGIN_TYPE_CALL_FOUR_BEHIND :
            CODE_BLOCK_ORIGIN_TYPE_JUMP;
}

void set_os_type_in_code_block_origin(struct CodeBlockOrigin *origin) {
    origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_OS;
}

DEFINE_STRUCT_LIST_METHODS(CodeBlock, code_block, block, start, 8, 64)

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

int add_interruption_type_code_block_origin(struct CodeBlock *block, struct Registers *regs, struct GlobalVariableWordValueMap *var_values) {
    int error_code;
    struct CodeBlockOriginList *origin_list = &block->origin_list;
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

int add_call_two_behind_type_code_block_origin(struct CodeBlock *block) {
    int error_code;
    struct CodeBlockOriginList *origin_list = &block->origin_list;
    if (index_of_code_block_origin_of_type_call_two_behind(origin_list) < 0) {
        struct CodeBlockOrigin *new_origin = prepare_new_code_block_origin(origin_list);
        if (!new_origin) {
            return 1;
        }

        new_origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND;
        make_all_registers_undefined(&new_origin->regs);
        initialize_global_variable_word_value_map(&new_origin->var_values);
        if ((error_code = insert_sorted_code_block_origin(origin_list, new_origin))) {
            return error_code;
        }
    }

    return 0;
}

int add_call_three_behind_type_code_block_origin(struct CodeBlock *block) {
    int error_code;
    struct CodeBlockOriginList *origin_list = &block->origin_list;
    if (index_of_code_block_origin_of_type_call_three_behind(origin_list) < 0) {
        struct CodeBlockOrigin *new_origin = prepare_new_code_block_origin(origin_list);
        if (!new_origin) {
            return 1;
        }

        new_origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND;
        make_all_registers_undefined(&new_origin->regs);
        initialize_global_variable_word_value_map(&new_origin->var_values);
        if ((error_code = insert_sorted_code_block_origin(origin_list, new_origin))) {
            return error_code;
        }
    }

    return 0;
}

int add_call_four_behind_type_code_block_origin(struct CodeBlock *block) {
    int error_code;
    struct CodeBlockOriginList *origin_list = &block->origin_list;
    if (index_of_code_block_origin_of_type_call_four_behind(origin_list) < 0) {
        struct CodeBlockOrigin *new_origin = prepare_new_code_block_origin(origin_list);
        if (!new_origin) {
            return 1;
        }

        new_origin->instruction = CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND;
        make_all_registers_undefined(&new_origin->regs);
        initialize_global_variable_word_value_map(&new_origin->var_values);
        if ((error_code = insert_sorted_code_block_origin(origin_list, new_origin))) {
            return error_code;
        }
    }

    return 0;
}

int add_jump_type_code_block_origin(struct CodeBlock *block, const char *origin_instruction, struct Registers *regs, struct GlobalVariableWordValueMap *var_values) {
    int error_code;
    struct CodeBlockOriginList *origin_list;
    int index;

    assert(origin_instruction != CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_OS);
    assert(origin_instruction != CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_INTERRUPTION);
    assert(origin_instruction != CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CONTINUE);
    assert(origin_instruction != CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND);
    assert(origin_instruction != CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND);
    assert(origin_instruction != CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND);

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

DEFINE_STRUCT_LIST_METHODS(CodeBlockOrigin, code_block_origin, origin, instruction, 8, 4)

int index_of_code_block_origin_of_type_call_two_behind(struct CodeBlockOriginList *list) {
    return index_of_code_block_origin_with_instruction(list, CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_TWO_BEHIND);
}

int index_of_code_block_origin_of_type_call_three_behind(struct CodeBlockOriginList *list) {
    return index_of_code_block_origin_with_instruction(list, CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_THREE_BEHIND);
}

int index_of_code_block_origin_of_type_call_four_behind(struct CodeBlockOriginList *list) {
    return index_of_code_block_origin_with_instruction(list, CODE_BLOCK_ORIGIN_INSTRUCTION_VALUE_TYPE_CALL_FOUR_BEHIND);
}
