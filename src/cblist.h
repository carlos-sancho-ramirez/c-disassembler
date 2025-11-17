#ifndef _CODE_BLOCK_LIST_H_
#define _CODE_BLOCK_LIST_H_

#include "cblock.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(CodeBlock, block);
DECLARE_STRUCT_LIST_METHODS(CodeBlock, cblock, block, start);

#endif /* _CODE_BLOCK_LIST_H_ */
