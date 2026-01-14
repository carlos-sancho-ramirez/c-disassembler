#ifndef _FINDER_H_
#define _FINDER_H_

#include "srresult.h"
#include "cblist.h"
#include "gvlist.h"
#include "sslist.h"
#include "reflist.h"
#include "printu.h"

int find_cblocks_and_gvars(
	struct SegmentReadResult *read_result,
	struct FilePrinter *printer_err,
	struct CodeBlockList *code_block_list,
	struct GlobalVariableList *global_variable_list,
	struct SegmentStartList *segment_start_list,
	struct ReferenceList *reference_list);

#endif /* _FINDER_H_ */
