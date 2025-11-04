#include "refs.h"
#include <assert.h>

struct GlobalVariable *get_gvar_from_ref_target(struct Reference *ref) {
	return ((ref->flags & REF_FLAG_TARGET_TYPE_MASK) == REF_FLAG_TARGET_IS_GVAR)?
			(struct GlobalVariable *) ref->target : NULL;
}

struct CodeBlock *get_cblock_from_ref_target(struct Reference *ref) {
	return ((ref->flags & REF_FLAG_TARGET_TYPE_MASK) == REF_FLAG_TARGET_IS_CBLOCK)?
			(struct CodeBlock *) ref->target : NULL;
}

void set_gvar_ref_from_instruction_address(struct Reference *ref, struct GlobalVariable *var) {
	assert(var);
	ref->flags = REF_FLAG_TARGET_IS_GVAR | REF_FLAG_IN_INSTRUCTION_ADDRESS;
	ref->target = var;
}

void set_gvar_ref_from_instruction_immediate_value(struct Reference *ref, struct GlobalVariable *var) {
	assert(var);
	ref->flags = REF_FLAG_TARGET_IS_GVAR | REF_FLAG_IN_INSTRUCTION_IMMEDIATE_VALUE;
	ref->target = var;
}

void set_cblock_ref_from_instruction_immediate_value(struct Reference *ref, struct CodeBlock *block) {
	assert(block);
	ref->flags = REF_FLAG_TARGET_IS_CBLOCK;
	ref->target = block;
}

DEFINE_STRUCT_LIST_METHODS(Reference, reference, reference, instruction, 8, 256)
