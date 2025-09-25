#include "global_variable_references.h"

void initialize_global_variable_reference_list(struct GlobalVariableReferenceList *list) {
	list->page_array_granularity = 8;
	list->references_per_page = 256;
	list->reference_count = 0;
	list->page_array = NULL;
	list->sorted_references = NULL;
}

int index_of_global_variable_reference_with_opcode_reference(const struct GlobalVariableReferenceList *list, const char *position) {
	int first = 0;
	int last = list->reference_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_position = list->sorted_references[index]->opcode_reference;
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

struct GlobalVariableReference *prepare_new_global_variable_reference(struct GlobalVariableReferenceList *list) {
	if ((list->reference_count % list->references_per_page) == 0) {
		if ((list->reference_count % (list->references_per_page * list->page_array_granularity)) == 0) {
			const int new_page_array_length = (list->reference_count / (list->references_per_page * list->page_array_granularity)) + list->page_array_granularity;
			list->page_array = realloc(list->page_array, new_page_array_length * sizeof(struct GlobalVariableReference *));
			if (!(list->page_array)) {
				return NULL;
			}

			list->sorted_references = realloc(list->sorted_references, new_page_array_length * list->references_per_page * sizeof(struct GlobalVariableReference *));
			if (!(list->sorted_references)) {
				return NULL;
			}
		}

		struct GlobalVariableReference *new_page = malloc(list->references_per_page * sizeof(struct GlobalVariableReference));
		if (!new_page) {
			return NULL;
		}

		list->page_array[list->reference_count / list->references_per_page] = new_page;
	}

	return list->page_array[list->reference_count / list->references_per_page] + (list->reference_count % list->references_per_page);
}

int insert_sorted_global_variable_reference(struct GlobalVariableReferenceList *list, struct GlobalVariableReference *new_variable) {
	int first = 0;
	int last = list->reference_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_position = list->sorted_references[index]->opcode_reference;
		if (this_position < new_variable->opcode_reference) {
			first = index + 1;
		}
		else if (this_position > new_variable->opcode_reference) {
			last = index;
		}
		else {
			return -1;
		}
	}

	for (int i = list->reference_count; i > last; i--) {
		list->sorted_references[i] = list->sorted_references[i - 1];
	}

	list->sorted_references[last] = new_variable;
	list->reference_count++;

	return 0;
}

void clear_global_variable_reference_list(struct GlobalVariableReferenceList *list) {
	if (list->reference_count > 0) {
		const int allocated_pages = (list->reference_count + list->references_per_page - 1) / list->references_per_page;
		for (int i = allocated_pages - 1; i >= 0; i--) {
			free(list->page_array[i]);
		}

		free(list->page_array);
		free(list->sorted_references);
		list->page_array = NULL;
		list->sorted_references = NULL;
		list->reference_count = 0;
	}
}
