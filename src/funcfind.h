#ifndef _FUNCTION_FINDER_H_
#define _FUNCTION_FINDER_H_

#include "funclist.h"

int find_functions(
		struct CodeBlock **blocks,
		unsigned int block_count,
		struct FunctionList *func_list);

#endif /* _FUNCTION_FINDER_H_ */
