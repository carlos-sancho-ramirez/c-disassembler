#ifndef _DUMPERS_H_
#define _DUMPERS_H_

#include "cblock.h"
#include "gvar.h"
#include "ref.h"
#include "funclist.h"
#include "printu.h"

int dump(
	const char *buffer,
	unsigned int buffer_origin,
	struct CodeBlock **sorted_blocks,
	unsigned int code_block_count,
	struct GlobalVariable **global_variables,
	unsigned int global_variable_count,
	const char **segment_starts,
	unsigned int segment_start_count,
	struct Reference **global_variable_references,
	unsigned int global_variable_reference_count,
	const char **sorted_relocations,
	unsigned int relocation_count,
	struct FunctionList *func_list,
	struct FilePrinter *print_out,
	struct FilePrinter *print_error);

#endif
