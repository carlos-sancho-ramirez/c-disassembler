#include "ref.h"

void initialize_ref(struct Reference *ref, const void *target, unsigned int flags, const char *instruction) {
	ref->target = target;
	ref->flags = flags;
	ref->instruction = instruction;
}
