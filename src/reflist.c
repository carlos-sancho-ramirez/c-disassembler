#include "reflist.h"

static void log_ref_insertion(struct Reference *ref) {
	/* Log entry to be added when required */
}

DEFINE_STRUCT_LIST_METHODS(Reference, ref, reference, instruction, 8, 256)
