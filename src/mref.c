#include "mref.h"

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

void initialize_mref_as_gvar_instruction_immediate_value(struct MutableReference *ref, struct GlobalVariable *var, const char *instruction) {
	assert(var && instruction);
	ref->flags = REF_FLAG_TARGET_IS_GVAR | REF_FLAG_IN_INSTRUCTION_IMMEDIATE_VALUE;
	ref->target = var;
	ref->instruction = instruction;
}

void initialize_mref_as_gvar_instruction_address(struct MutableReference *ref, struct GlobalVariable *var, const char *instruction) {
	assert(var && instruction);
	ref->flags = REF_FLAG_TARGET_IS_GVAR | REF_FLAG_IN_INSTRUCTION_ADDRESS;
	ref->target = var;
	ref->instruction = instruction;
}

void initialize_mref_as_cblock_instruction_immediate_value(struct MutableReference *ref, struct MutableCodeBlock *block, const char *instruction) {
	assert(block && instruction);
	ref->flags = REF_FLAG_TARGET_IS_CBLOCK;
	ref->target = block;
	ref->instruction = instruction;
}

const char *get_mref_instruction(struct MutableReference *ref) {
	return ref->instruction;
}

struct GlobalVariable *get_gvar_from_mref_target(const struct MutableReference *ref) {
	return ((ref->flags & REF_FLAG_TARGET_TYPE_MASK) == REF_FLAG_TARGET_IS_GVAR)?
			(struct GlobalVariable *) ref->target : NULL;
}

struct MutableCodeBlock *get_mcblock_from_mref_target(const struct MutableReference *ref) {
	return ((ref->flags & REF_FLAG_TARGET_TYPE_MASK) == REF_FLAG_TARGET_IS_CBLOCK)?
			(struct MutableCodeBlock *) ref->target : NULL;
}

int is_mref_in_instruction_address(const struct MutableReference *ref) {
	return (ref->flags & REF_FLAG_WHERE_IN_INSTRUCTION_MASK) == REF_FLAG_IN_INSTRUCTION_ADDRESS;
}

void set_gvar_mref_read_access(struct MutableReference *ref) {
	ref->flags |= REF_FLAG_ACCESS_READ;
}

void set_gvar_mref_write_access(struct MutableReference *ref) {
	ref->flags |= REF_FLAG_ACCESS_WRITE;
}
