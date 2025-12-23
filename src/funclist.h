#ifndef _FUNCTION_LIST_H_
#define _FUNCTION_LIST_H_

#include "function.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(Function, func);
DECLARE_STRUCT_LIST_METHODS(Function, func, func, start);

int index_of_func_containing_block_start(struct FunctionList *list, const char *start);

#ifdef DEBUG
void print_funclist(const struct FunctionList *list);
#endif /* DEBUG */

#endif /* _FUNCTION_H_ */
