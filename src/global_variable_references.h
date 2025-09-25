#ifndef _GLOBAL_VARIABLE_REFERENCES_H_
#define _GLOBAL_VARIABLE_REFERENCES_H_

#include "global_variables.h"

struct GlobalVariableReference {
	const char *opcode_reference;
	struct GlobalVariable *variable;
};

/**
 * Complex structure containing pages of global variable references.
 * It is designed to grow as more references are added to it.
 */
struct GlobalVariableReferenceList {
	/**
	 * Reflects that the page_array allocated size must be always a multiple of this value.
	 * The bigger, the more we will avoid calling realloc for each new inserted reference, improving its performance.
	 * The smaller, the less memory this list will use.
	 * This parameter should be adjusted according to the environment where this executable should run.
	 */
	unsigned int page_array_granularity;

	/**
	 * Number of variables in each page.
	 * The bigger, the more we will avoid calling realloc for each new inserted variable and the less
	 * the page_array will grow, improving its performance.
	 * This parameter should be adjusted according to the environment where this executable should run.
	 */
	unsigned int references_per_page;

	/**
	 * Array holding all allocated pages in the order they have been allocated.
	 * The allocated size of this array can be calculated combining the values in reference_count,
	 * references_per_page and page_array_granularity.
	 * This will be NULL when reference_count is 0.
	 */
	struct GlobalVariableReference **page_array;

	/**
	 * Array pointing to all global variable references, sorted by its opcode_reference.
	 * The allocated size of this array can be calculated combining the values in reference_count,
	 * references_per_page and page_array_granularity.
	 * This will be NULL when reference_count is 0.
	 */
	struct GlobalVariableReference **sorted_references;

	/**
	 * Number of references already inserted.
	 * Note that this value is most of the times lower than the actual capacity allocated in memory
	 * to hold all of them.
	 */
	unsigned int reference_count;
};

/**
 * Set all its values. After this, this list will be empty, but ready.
 */
void initialize_global_variable_reference_list(struct GlobalVariableReferenceList *list);

/**
 * Searches for a variable whose start matches the given one.
 * This will return the index of the variable within the sorted_variables array. Or -1 if none matches.
 */
int index_of_global_variable_reference_with_opcode_reference(const struct GlobalVariableReferenceList *list, const char *opcode_reference);

/**
 * Returns a proper pointer to store a new reference.
 * The returned pointer should be filled and call insert_sorted_global_variable_reference method in order to sort it properly.
 * This method may require allocating a new page of memory.
 * This method will return NULL in case of failure.
 */
struct GlobalVariableReference *prepare_new_global_variable_reference(struct GlobalVariableReferenceList *list);

/**
 * Inserts the global variable reference returned previously by prepare_new_global_variable_reference.
 * This will increase the reference_count value by 1 and will update the sorted_references inserting the
 * reference to this variable in the suitable position.
 */
int insert_sorted_global_variable_reference(struct GlobalVariableReferenceList *list, struct GlobalVariableReference *new_reference);

/**
 * Free all the allocated references and restores this list to its initial state.
 */
void clear_global_variable_reference_list(struct GlobalVariableReferenceList *list);

#endif // _GLOBAL_VARIABLE_REFERENCES_H_
