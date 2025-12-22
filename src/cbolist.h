#ifndef _CODE_BLOCK_ORIGIN_LIST_H_
#define _CODE_BLOCK_ORIGIN_LIST_H_

#include "cborigin.h"
#include "slmacros.h"

/**
 * Complex structure containing pages of the given struct in CodeBlockOrigin.
 * It is designed to grow as more references are added to it.
 *
 * Regarding the origins within the list, they will be sorted first by its type,
 * and later by its instruction (only in case of JUMP type) or its behind count
 * (only in case of CALL RETURN type).
 *
 * Note that it is not possible to have multiple origins of types OS,
 * INTERRUPTION or CONTINUE within the same list.
 */
struct CodeBlockOriginList {
	/**
	 * Array holding all allocated pages in the order they have been allocated.
	 * The allocated size of this array can be calculated combining the values in origin_count,
	 * origins_per_page and page_array_granularity.
	 * This will be NULL when origin_count is 0.
	 */
	struct CodeBlockOrigin **page_array;

	/**
	 * Array pointing to all structs, sorted by its pointer.
	 * The allocated size of this array can be calculated combining the values in origin_count,
	 * origins_per_page and page_array_granularity.
	 * This will be NULL when reference_count is 0.
	 */
	struct CodeBlockOrigin **sorted_origins;

	/**
	 * Number of structs already inserted.
	 * Note that this value is most of the times lower than the actual capacity allocated in memory
	 * to hold all of them.
	 */
	unsigned int origin_count;
};

DECLARE_STRUCT_LIST_METHODS(CodeBlockOrigin, cborigin, origin, instruction);

/**
 * Searches for a origin whose type is interruption.
 * This will return the index of the origin within the sorted_origins array. Or -1 if none matches.
 */
int index_of_cborigin_with_type_interruption(const struct CodeBlockOriginList *list);

int index_of_cborigin_of_type_continue(const struct CodeBlockOriginList *list);
int index_of_first_cborigin_of_type_call_return(const struct CodeBlockOriginList *list);
int index_of_cborigin_of_type_call_return(const struct CodeBlockOriginList *list, unsigned int behind_count);

void accumulate_registers_from_cbolist(struct Registers *regs, const struct CodeBlockOriginList *list);
int accumulate_stack_from_cbolist(struct Stack *stack, const struct CodeBlockOriginList *list);
int accumulate_gvwvmap_from_cbolist(struct GlobalVariableWordValueMap *map, const struct CodeBlockOriginList *list);

int add_call_return_type_cborigin(struct CodeBlockOriginList *list, const struct Registers *regs, const struct Stack *stack, unsigned int behind_count);

#endif /* _CODE_BLOCK_ORIGIN_LIST_H_ */
