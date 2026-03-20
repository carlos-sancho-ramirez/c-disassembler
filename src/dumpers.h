#ifndef _DUMPERS_H_
#define _DUMPERS_H_

#include "pcontent.h"
#include "funclist.h"
#include "printu.h"

int dump(
	const char *buffer,
	unsigned int buffer_origin,
	const struct ProgramContent *pcontent,
	const char **segment_starts,
	unsigned int segment_start_count,
	const char **sorted_relocations,
	unsigned int relocation_count,
	struct FunctionList *func_list,
	struct FilePrinter *print_out,
	struct FilePrinter *print_error);

#endif
