#include "code_blocks.h"

#define CODE_BLOCK_FLAG_EVALUATED_AT_LEAST_ONCE 1
#define CODE_BLOCK_FLAG_VALID_EVALUATION 2
#define CODE_BLOCK_FLAG_UNDER_EVALUATION 4

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

DEFINE_STRUCT_LIST_METHODS(CodeBlock, code_block, block, start, 8, 64)

void accumulate_registers_from_code_block_origin_list(struct Registers *regs, struct CodeBlockOriginList *origin_list) {
    if (origin_list->origin_count) {
        copy_registers(regs, &origin_list->sorted_origins[0]->regs);
        for (int i = 0; i < origin_list->origin_count; i++) {
            merge_registers(regs, &origin_list->sorted_origins[i]->regs);
        }
    }
}

int add_code_block_origin(struct CodeBlock *block, const char *origin_instruction, struct CodeBlock *origin_block, struct Registers *regs) {
    int error_code;
    struct CodeBlockOriginList *origin_list = &block->origin_list;
    int index = index_of_code_block_origin_with_instruction(origin_list, origin_instruction);
    if (index < 0) {
        struct Registers accumulated_regs;
        if (origin_list->origin_count) {
            accumulate_registers_from_code_block_origin_list(&accumulated_regs, origin_list);
        }

        struct CodeBlockOrigin *new_origin = prepare_new_code_block_origin(origin_list);
        new_origin->block = origin_block;
        new_origin->instruction = origin_instruction;
        copy_registers(&new_origin->regs, regs);
        if ((error_code = insert_sorted_code_block_origin(origin_list, new_origin))) {
            return error_code;
        }

        if (origin_list->origin_count > 1 && changes_on_merging_registers(&accumulated_regs, regs)) {
            invalidate_code_block_check(block);
        } 
    }
    else {
        struct CodeBlockOrigin *origin = origin_list->sorted_origins[index];
        if (origin->block == origin_block && changes_on_merging_registers(&origin->regs, regs)) {
            merge_registers(&origin->regs, regs);
            invalidate_code_block_check(block);
        }
    }

    return 0;
}

DEFINE_STRUCT_LIST_METHODS(CodeBlockOrigin, code_block_origin, origin, instruction, 8, 4)
