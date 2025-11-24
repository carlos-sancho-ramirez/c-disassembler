#include "cblist.h"
#include "printd.h"

static void log_cblock_insertion(struct CodeBlock *block) {
	DEBUG_PRINT2(" Registering new code block at +%x:%x\n", block->relative_cs, block->ip);
}

DEFINE_STRUCT_LIST_METHODS(CodeBlock, cblock, block, start, 8, 64)
