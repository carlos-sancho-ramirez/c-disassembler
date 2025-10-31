#ifndef _DUMPERS_H_
#define _DUMPERS_H_

#include "cblock.h"
#include "gvar.h"
#include "refs.h"

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
	void (*print)(const char *),
	void (*print_error)(const char *),
	void (*print_segment_start_label)(void (*)(const char *), const char *),
	void (*print_code_label)(void (*)(const char *), int, int),
	void (*print_variable_label)(void (*)(const char *), unsigned int));

#endif
