#include "refs.h"
#include <assert.h>

struct GlobalVariable *get_global_variable_from_reference_target(struct Reference *ref) {
    return ((ref->flags & REFERENCE_FLAG_TARGET_TYPE_MASK) == REFERENCE_FLAG_TARGET_IS_GLOBAL_VARIABLE)?
            (struct GlobalVariable *) ref->target : NULL;
}

struct CodeBlock *get_code_block_from_reference_target(struct Reference *ref) {
    return ((ref->flags & REFERENCE_FLAG_TARGET_TYPE_MASK) == REFERENCE_FLAG_TARGET_IS_CODE_BLOCK)?
            (struct CodeBlock *) ref->target : NULL;
}

void set_global_variable_reference_from_instruction_address(struct Reference *ref, struct GlobalVariable *var) {
    assert(var);
    ref->flags = REFERENCE_FLAG_TARGET_IS_GLOBAL_VARIABLE | REFERENCE_FLAG_IN_INSTRUCTION_ADDRESS;
    ref->target = var;
}

void set_global_variable_reference_from_instruction_immediate_value(struct Reference *ref, struct GlobalVariable *var) {
    assert(var);
    ref->flags = REFERENCE_FLAG_TARGET_IS_GLOBAL_VARIABLE | REFERENCE_FLAG_IN_INSTRUCTION_IMMEDIATE_VALUE;
    ref->target = var;
}

void set_code_block_reference_from_instruction_immediate_value(struct Reference *ref, struct CodeBlock *block) {
    assert(block);
    ref->flags = REFERENCE_FLAG_TARGET_IS_CODE_BLOCK;
    ref->target = block;
}

DEFINE_STRUCT_LIST_METHODS(Reference, reference, reference, instruction, 8, 256)
