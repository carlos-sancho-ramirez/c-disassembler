#ifndef _MUTABLE_REFERENCE_LIST_H_
#define _MUTABLE_REFERENCE_LIST_H_

#include "mref.h"

DEFINE_STRUCT_LIST(MutableReference, reference);
DECLARE_STRUCT_LIST_METHODS(MutableReference, ref, reference, instruction);
DECLARE_STRUCT_LIST_INSERT_METHOD(MutableReference, ref, reference);

#endif /* _MUTABLE_REFERENCE_LIST_H_ */
