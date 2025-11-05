#include "ref.h"

#define REF_FLAG_TARGET_TYPE_MASK 1
#define REF_FLAG_TARGET_IS_GVAR 0
#define REF_FLAG_TARGET_IS_CBLOCK 1

#define REF_FLAG_WHERE_IN_INSTRUCTION_MASK 2
#define REF_FLAG_IN_INSTRUCTION_ADDRESS 2
#define REF_FLAG_IN_INSTRUCTION_IMMEDIATE_VALUE 0

#define REF_FLAG_ACCESS_MASK 0x0C
#define REF_FLAG_ACCESS_READ 0x04
#define REF_FLAG_ACCESS_WRITE 0x08

#include <assert.h>

struct GlobalVariable *get_gvar_from_ref_target(const struct Reference *ref) {
	return ((ref->flags & REF_FLAG_TARGET_TYPE_MASK) == REF_FLAG_TARGET_IS_GVAR)?
			(struct GlobalVariable *) ref->target : NULL;
}

struct CodeBlock *get_cblock_from_ref_target(const struct Reference *ref) {
	return ((ref->flags & REF_FLAG_TARGET_TYPE_MASK) == REF_FLAG_TARGET_IS_CBLOCK)?
			(struct CodeBlock *) ref->target : NULL;
}

int is_ref_in_instruction_address(const struct Reference *ref) {
	return (ref->flags & REF_FLAG_WHERE_IN_INSTRUCTION_MASK) == REF_FLAG_IN_INSTRUCTION_ADDRESS;
}

void set_gvar_ref_from_instruction_address(struct Reference *ref, struct GlobalVariable *var) {
	assert(var);
	ref->flags = REF_FLAG_TARGET_IS_GVAR | REF_FLAG_IN_INSTRUCTION_ADDRESS;
	ref->target = var;
}

void set_gvar_ref_read_access(struct Reference *ref) {
	ref->flags |= REF_FLAG_ACCESS_READ;
}

void set_gvar_ref_write_access(struct Reference *ref) {
	ref->flags |= REF_FLAG_ACCESS_WRITE;
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
