#ifndef _GLOBAL_VARIABLES_H_
#define _GLOBAL_VARIABLES_H_

#include <stdlib.h>

#define GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING 5

struct GlobalVariable {
	const char *start;
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
};

/**
 * Complex structure containing pages of global variables.
 * It is designed to grow as more variables are added to it.
 */
struct GlobalVariableList {
	/**
	 * Reflects that the page_array allocated size must be always a multiple of this value.
	 * The bigger, the more we will avoid calling realloc for each new inserted variable, improving its performance.
	 * The smaller, the less memory this list will use.
	 * This parameter should be adjusted according to the environtment where this executable should run.
	 */
	unsigned int page_array_granularity;

	/**
	 * Number of variables in each page.
	 * The bigger, the more we will avoid calling realloc for each new inserted variable and the less
	 * the page_array will grow, improving its performance.
	 * This parameter should be adjusted according to the environtment where this executable should run.
	 */
	unsigned int variables_per_page;

	/**
	 * Array holding all allocated pages in the order they have been allocated.
	 * The allocated size of this array can be calculated combining the values in variable_count,
	 * variables_per_page and page_array_granularity.
	 * This will be NULL when variable_count is 0.
	 */
	struct GlobalVariable **page_array;

	/**
	 * Array pointing to all global variables, sorted by its start.
	 * The allocated size of this array can be calculated combining the values in variable_count,
	 * variables_per_page and page_array_granularity.
	 * This will be NULL when variable_count is 0.
	 */
	struct GlobalVariable **sorted_variables;

	/**
	 * Number of variables already inserted.
	 * Note that this value is most of the times lower than the actual capacity allocated in memory
	 * to hold all of them.
	 */
	unsigned int variable_count;
};

/**
 * Set all its values. After this, this list will be empty, but ready.
 */
void initialize_global_variable_list(struct GlobalVariableList *list);

/**
 * Searches for a variable whose start matches the given one.
 * This will return the index of the variable within the sorted_variables array. Or -1 if none matches.
 */
int index_of_global_variable_with_start(const struct GlobalVariableList *list, const char *start);

/**
 * Returns a proper pointer to store a new variable.
 * The returned pointer should be filled and call insert_sorted_global_variable method in order to sort it properly.
 * This method may require allocating a new page of memory.
 * This method will return NULL in case of failure.
 */
struct GlobalVariable *prepare_new_global_variable(struct GlobalVariableList *list);

/**
 * Inserts the global variable returned previously by prepare_new_global_variable.
 * This will increase the variable_count value by 1 and will update the sorted_variables inserting the
 * reference to this variable in the suitable position.
 */
int insert_sorted_global_variable(struct GlobalVariableList *list, struct GlobalVariable *new_variable);

/**
 * Free all the allocated variables and restores this list to its initial state.
 */
void clear_global_variable_list(struct GlobalVariableList *list);

#endif // _GLOBAL_VARIABLES_H_