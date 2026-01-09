#ifndef _FUNCTION_LIST_H_
#define _FUNCTION_LIST_H_

#include "function.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(Function, func);

DECLARE_STRUCT_LIST_INITIALIZE_METHOD(Function, func);
DECLARE_STRUCT_LIST_GET_UNSORTED_METHOD(Function, func);
DECLARE_STRUCT_LIST_PREPARE_NEW_METHOD(Function, func);
DECLARE_STRUCT_LIST_CLEAR_METHOD(Function, func);

struct Function *get_unsorted_func(const struct FunctionList *list, int index);

/**
 * Searches for a function containing a block whose start matches the given one.
 * This will return the index of the function within the sorted array. Or -1 if none matches.
 */
int index_of_func_containing_block_start(const struct FunctionList *list, const char *start);

/**
 * Inserts the short_item_name returned previously by prepare_new_func.
 * This will increase the func_count value by 1 and will update the sorted_funcs inserting the
 * reference to this short_item_name in its suitable position.
 */
int insert_func(struct FunctionList *list, struct Function *new_func);

#ifdef DEBUG
void print_funclist(const struct FunctionList *list);
#endif /* DEBUG */

#endif /* _FUNCTION_H_ */
