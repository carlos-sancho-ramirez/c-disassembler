#include "global_variables.h"

void initialize_global_variable_list(struct GlobalVariableList *list) {
	list->page_array_granularity = 8;
	list->variables_per_page = 256;
	list->variable_count = 0;
	list->page_array = NULL;
	list->sorted_variables = NULL;
}

int index_of_global_variable_with_start(const struct GlobalVariableList *list, const char *position) {
	int first = 0;
	int last = list->variable_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_position = list->sorted_variables[index]->start;
		if (this_position < position) {
			first = index + 1;
		}
		else if (this_position > position) {
			last = index;
		}
		else {
			return index;
		}
	}

	return -1;
}

struct GlobalVariable *prepare_new_global_variable(struct GlobalVariableList *list) {
	if ((list->variable_count % list->variables_per_page) == 0) {
		if ((list->variable_count % (list->variables_per_page * list->page_array_granularity)) == 0) {
			const int new_page_array_length = (list->variable_count / (list->variables_per_page * list->page_array_granularity)) + list->page_array_granularity;
			list->page_array = realloc(list->page_array, new_page_array_length * sizeof(struct GlobalVariable *));
			if (!(list->page_array)) {
				return NULL;
			}

			list->sorted_variables = realloc(list->sorted_variables, new_page_array_length * list->variables_per_page * sizeof(struct GlobalVariable *));
			if (!(list->sorted_variables)) {
				return NULL;
			}
		}

		struct GlobalVariable *new_page = malloc(list->variables_per_page * sizeof(struct GlobalVariable));
		if (!new_page) {
			return NULL;
		}

		list->page_array[list->variable_count / list->variables_per_page] = new_page;
	}

	return list->page_array[list->variable_count / list->variables_per_page] + (list->variable_count % list->variables_per_page);
}

int insert_sorted_global_variable(struct GlobalVariableList *list, struct GlobalVariable *new_variable) {
	int first = 0;
	int last = list->variable_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_position = list->sorted_variables[index]->start;
		if (this_position < new_variable->start) {
			first = index + 1;
		}
		else if (this_position > new_variable->start) {
			last = index;
		}
		else {
			return -1;
		}
	}

	for (int i = list->variable_count; i > last; i--) {
		list->sorted_variables[i] = list->sorted_variables[i - 1];
	}

	list->sorted_variables[last] = new_variable;
	list->variable_count++;

	return 0;
}

void clear_global_variable_list(struct GlobalVariableList *list) {
	if (list->variable_count > 0) {
		const int allocated_pages = (list->variable_count + list->variables_per_page - 1) / list->variables_per_page;
		for (int i = allocated_pages - 1; i >= 0; i--) {
			free(list->page_array[i]);
		}

		free(list->page_array);
		free(list->sorted_variables);
		list->page_array = NULL;
		list->sorted_variables = NULL;
		list->variable_count = 0;
	}
}
