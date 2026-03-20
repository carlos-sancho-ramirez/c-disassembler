#include "ref.h"
#include "refdefs.h"

#include <stdlib.h>

void initialize_ref(struct Reference *ref, const void *target, unsigned int flags, const char *instruction) {
	ref->target = target;
	ref->flags = flags;
	ref->instruction = instruction;
}

const char *get_ref_instruction(const struct Reference *ref) {
	return ref->instruction;
}

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
