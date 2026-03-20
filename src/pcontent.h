#ifndef _PROGRAM_CONTENT_H_
#define _PROGRAM_CONTENT_H_

#include "cblock.h"
#include "gvlist.h"
#include "mreflist.h"

struct ProgramContent {
	unsigned int block_count;
	const struct CodeBlock *blocks;
	const struct GlobalVariableList *vars;
	const struct MutableReferenceList *refs;
};

void initialize_pcontent(
		struct ProgramContent *pcontent,
		unsigned int block_count,
		const struct CodeBlock *blocks,
		const struct GlobalVariableList *vars,
		const struct MutableReferenceList *refs);

unsigned int get_pcontent_block_count(const struct ProgramContent *pcontent);
const struct CodeBlock *get_pcontent_blocks(const struct ProgramContent *pcontent);
const struct GlobalVariableList *get_pcontent_vars(const struct ProgramContent *pcontent);
const struct MutableReferenceList *get_pcontent_refs(const struct ProgramContent *pcontent);

#endif /* _PROGRAM_CONTENT_H_ */
