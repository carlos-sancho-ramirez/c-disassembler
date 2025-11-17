#ifndef _REFERENCE_LIST_H_
#define _REFERENCE_LIST_H_

#include "ref.h"

DEFINE_STRUCT_LIST(Reference, reference);
DECLARE_STRUCT_LIST_METHODS(Reference, ref, reference, instruction);

#endif /* _REFERENCE_LIST_H_ */
