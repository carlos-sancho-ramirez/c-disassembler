#ifndef _DUMPERS_H_
#define _DUMPERS_H_

#include "code_blocks.h"
#include "global_variables.h"

int dump(
    struct CodeBlock **sorted_blocks,
    unsigned int code_block_count,
    struct GlobalVariable **global_variables,
    unsigned int global_variable_count,
    void (*print)(const char *),
    void (*print_error)(const char *));

#endif // _DUMPERS_H_
