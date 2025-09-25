#ifndef _DUMPERS_H_
#define _DUMPERS_H_

#include "code_blocks.h"
#include "global_variables.h"
#include "global_variable_references.h"

int dump(
    struct CodeBlock **sorted_blocks,
    unsigned int code_block_count,
    struct GlobalVariable **global_variables,
    unsigned int global_variable_count,
    struct GlobalVariableReference **global_variable_references,
    unsigned int global_variable_reference_count,
    void (*print)(const char *),
    void (*print_error)(const char *),
    void (*print_code_label)(void (*)(const char *), int, int),
    void (*print_variable_label)(void (*)(const char *), unsigned int));

#endif // _DUMPERS_H_
