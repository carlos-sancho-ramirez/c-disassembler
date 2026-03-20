#include "pcontent.h"

void initialize_pcontent(
		struct ProgramContent *pcontent,
		unsigned int block_count,
		const struct CodeBlock *blocks,
		const struct GlobalVariableList *vars,
		const struct ReferenceList *refs) {
	pcontent->block_count = block_count;
	pcontent->blocks = blocks;
	pcontent->vars = vars;
	pcontent->refs = refs;
}

unsigned int get_pcontent_block_count(const struct ProgramContent *pcontent) {
	return pcontent->block_count;
}

const struct CodeBlock *get_pcontent_blocks(const struct ProgramContent *pcontent) {
	return pcontent->blocks;
}

const struct GlobalVariableList *get_pcontent_vars(const struct ProgramContent *pcontent) {
	return pcontent->vars;
}

const struct ReferenceList *get_pcontent_refs(const struct ProgramContent *pcontent) {
	return pcontent->refs;
}
