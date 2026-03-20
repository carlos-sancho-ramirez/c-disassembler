#include "pcontent.h"

static void initialize_pcontent(
		struct ProgramContent *pcontent,
		const struct MutableCodeBlockList *blocks,
		const struct GlobalVariableList *vars,
		const struct ReferenceList *refs) {
	pcontent->blocks = blocks;
	pcontent->vars = vars;
	pcontent->refs = refs;
}

struct ProgramContent *new_pcontent(
		const struct MutableCodeBlockList *blocks,
		const struct GlobalVariableList *vars,
		const struct ReferenceList *refs) {
	struct ProgramContent *pcontent = malloc(sizeof(struct ProgramContent));
	initialize_pcontent(pcontent, blocks, vars, refs);
	return pcontent;
}

const struct MutableCodeBlockList *get_pcontent_blocks(const struct ProgramContent *pcontent) {
	return pcontent->blocks;
}

const struct GlobalVariableList *get_pcontent_vars(const struct ProgramContent *pcontent) {
	return pcontent->vars;
}

const struct ReferenceList *get_pcontent_refs(const struct ProgramContent *pcontent) {
	return pcontent->refs;
}
