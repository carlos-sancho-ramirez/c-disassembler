#ifndef _CODE_BLOCK_ORIGIN_LIST_H_
#define _CODE_BLOCK_ORIGIN_LIST_H_

#include "cborigin.h"
#include "slmacros.h"

DEFINE_STRUCT_LIST(CodeBlockOrigin, origin);
DECLARE_STRUCT_LIST_METHODS(CodeBlockOrigin, code_block_origin, origin, instruction);

int index_of_code_block_origin_of_type_call_two_behind(struct CodeBlockOriginList *list);
int index_of_code_block_origin_of_type_call_three_behind(struct CodeBlockOriginList *list);
int index_of_code_block_origin_of_type_call_four_behind(struct CodeBlockOriginList *list);

void accumulate_registers_from_code_block_origin_list(struct Registers *regs, struct CodeBlockOriginList *origin_list);

int accumulate_global_variable_word_values_from_code_block_origin_list(struct GlobalVariableWordValueMap *map, struct CodeBlockOriginList *origin_list);

int add_call_two_behind_type_code_block_origin(struct CodeBlockOriginList *list);
int add_call_three_behind_type_code_block_origin(struct CodeBlockOriginList *list);
int add_call_four_behind_type_code_block_origin(struct CodeBlockOriginList *list);

#endif /* _CODE_BLOCK_ORIGIN_LIST_H_ */
