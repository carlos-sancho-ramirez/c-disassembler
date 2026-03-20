#ifndef _PROGRAM_CONTENT_H_
#define _PROGRAM_CONTENT_H_

#include "mcblist.h"
#include "gvlist.h"
#include "reflist.h"

struct ProgramContent {
	const struct MutableCodeBlockList *blocks;
	const struct GlobalVariableList *vars;
	const struct ReferenceList *refs;
};

struct ProgramContent *new_pcontent(
		const struct MutableCodeBlockList *blocks,
		const struct GlobalVariableList *vars,
		const struct ReferenceList *refs);

const struct MutableCodeBlockList *get_pcontent_blocks(const struct ProgramContent *pcontent);
const struct GlobalVariableList *get_pcontent_vars(const struct ProgramContent *pcontent);
const struct ReferenceList *get_pcontent_refs(const struct ProgramContent *pcontent);

#endif /* _PROGRAM_CONTENT_H_ */
