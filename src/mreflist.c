#include "mreflist.h"

static void log_ref_insertion(struct MutableReference *ref) {
	/* Log entry to be added when required */
}

DEFINE_STRUCT_LIST_METHODS(MutableReference, ref, reference, instruction, 8, 256)
