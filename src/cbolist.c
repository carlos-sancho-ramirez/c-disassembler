#include "cbolist.h"
#include <assert.h>

#define PAGE_ARRAY_GRANULARITY 8
#define ORIGINS_PER_PAGE 4

void initialize_cbolist(struct CodeBlockOriginList *list) {
	list->origin_count = 0;
	list->page_array = NULL;
	list->sorted_origins = NULL;
}

int index_of_cborigin_with_type_interruption(const struct CodeBlockOriginList *list) {
	int first = 0;
	int last = list->origin_count;
	while (last > first) {
		int index = (first + last) / 2;
		struct CodeBlockOrigin *origin_at_index = list->sorted_origins[index];
		unsigned int origin_type = get_cborigin_type(origin_at_index);
		if (origin_type > CBORIGIN_TYPE_INTERRUPTION) {
			last = index;
		}
		else if (origin_type < CBORIGIN_TYPE_INTERRUPTION) {
			first = index + 1;
		}
		else {
			return index;
		}
	}

	return -1;
}

int index_of_cborigin_with_instruction(const struct CodeBlockOriginList *list, const char *instruction) {
	int first = 0;
	int last = list->origin_count;
	while (last > first) {
		int index = (first + last) / 2;
		const struct CodeBlockOrigin *this_origin = list->sorted_origins[index];
		const char *this_instruction = this_origin->instruction;
		const int this_origin_type = get_cborigin_type(this_origin);
		if (this_origin_type < CBORIGIN_TYPE_JUMP || this_origin_type == CBORIGIN_TYPE_JUMP && this_instruction < instruction) {
			first = index + 1;
		}
		else if (this_origin_type > CBORIGIN_TYPE_JUMP || this_origin_type == CBORIGIN_TYPE_JUMP && this_instruction > instruction) {
			last = index;
		}
		else {
			return index;
		}
	}

	return -1;
}

int index_of_cborigin_containing_position(const struct CodeBlockOriginList *list, const char *position) {
	int first = 0;
	int last = list->origin_count;
	while (last > first) {
		int index = (first + last) / 2;
		const struct CodeBlockOrigin *this_origin = list->sorted_origins[index];
		const char *this_instruction = this_origin->instruction;
		const int this_origin_type = get_cborigin_type(this_origin);
		if (this_origin_type < CBORIGIN_TYPE_JUMP || this_origin_type == CBORIGIN_TYPE_JUMP && this_instruction < position) {
			first = index + 1;
		}
		else if (this_origin_type > CBORIGIN_TYPE_JUMP || this_origin_type == CBORIGIN_TYPE_JUMP && this_instruction > position) {
			last = index;
		}
		else {
			return index;
		}
	}

	return (first > 0 && get_cborigin_type(list->sorted_origins[first - 1]) == CBORIGIN_TYPE_JUMP)? first - 1 : -1;
}

struct CodeBlockOrigin *prepare_new_cborigin(struct CodeBlockOriginList *list) {
	if ((list->origin_count % ORIGINS_PER_PAGE) == 0) {
		struct CodeBlockOrigin *new_page;
		if ((list->origin_count % (ORIGINS_PER_PAGE * PAGE_ARRAY_GRANULARITY)) == 0) {
			const int new_page_array_length = (list->origin_count / (ORIGINS_PER_PAGE * PAGE_ARRAY_GRANULARITY)) + PAGE_ARRAY_GRANULARITY;
			list->page_array = realloc(list->page_array, new_page_array_length * sizeof(struct CodeBlockOrigin *));
			if (!(list->page_array)) {
				return NULL;
			}

			list->sorted_origins = realloc(list->sorted_origins, new_page_array_length * ORIGINS_PER_PAGE * sizeof(struct CodeBlockOrigin *));
			if (!(list->sorted_origins)) {
				return NULL;
			}
		}

		new_page = malloc(ORIGINS_PER_PAGE * sizeof(struct CodeBlockOrigin));
		if (!new_page) {
			return NULL;
		}

		list->page_array[list->origin_count / ORIGINS_PER_PAGE] = new_page;
	}

	return list->page_array[list->origin_count / ORIGINS_PER_PAGE] + (list->origin_count % ORIGINS_PER_PAGE);
}

static int is_before(struct CodeBlockOrigin *a, struct CodeBlockOrigin *b) {
	unsigned int a_type = get_cborigin_type(a);
	unsigned int b_type = get_cborigin_type(b);

	if (a_type < b_type) {
		return 1;
	}
	else if (a_type > b_type) {
		return 0;
	}
	else if (a_type == CBORIGIN_TYPE_CALL_RETURN) {
		return get_cborigin_behind_count(a) < get_cborigin_behind_count(b);
	}
	else if (a_type == CBORIGIN_TYPE_JUMP) {
		return a->instruction < b->instruction;
	}
	else {
		return 0;
	}
}

int insert_cborigin(struct CodeBlockOriginList *list, struct CodeBlockOrigin *new_origin) {
	int first = 0;
	int last = list->origin_count;
	int i;
	while (last > first) {
		int index = (first + last) / 2;
		struct CodeBlockOrigin *origin_at_index = list->sorted_origins[index];
		if (is_before(origin_at_index, new_origin)) {
			first = index + 1;
		}
		else if (is_before(new_origin, origin_at_index)) {
			last = index;
		}
		else {
			return -1;
		}
	}

	for (i = list->origin_count; i > last; i--) {
		list->sorted_origins[i] = list->sorted_origins[i - 1];
	}

	list->sorted_origins[last] = new_origin;
	list->origin_count++;

	return 0;
}

void clear_cbolist(struct CodeBlockOriginList *list) {
	if (list->origin_count > 0) {
		const int allocated_pages = (list->origin_count + ORIGINS_PER_PAGE - 1) / ORIGINS_PER_PAGE;
		int i;
		for (i = allocated_pages - 1; i >= 0; i--) {
			free(list->page_array[i]);
		}

		free(list->page_array);
		free(list->sorted_origins);
		list->page_array = NULL;
		list->sorted_origins = NULL;
		list->origin_count = 0;
	}
}

void accumulate_registers_from_cbolist(struct Registers *regs, const struct CodeBlockOriginList *list) {
	if (list->origin_count) {
		int i;
		copy_registers(regs, &list->sorted_origins[0]->regs);
		for (i = 1; i < list->origin_count; i++) {
			merge_registers(regs, &list->sorted_origins[i]->regs);
		}
	}
}

int accumulate_stack_from_cbolist(struct Stack *stack, const struct CodeBlockOriginList *list) {
	if (list->origin_count) {
		int error_code;
		int i;
		if ((error_code = copy_stack(stack, &list->sorted_origins[0]->stack))) {
			return error_code;
		}

		for (i = 1; i < list->origin_count; i++) {
			merge_stacks(stack, &list->sorted_origins[i]->stack);
		}
	}

	return 0;
}

int accumulate_gvwvmap_from_cbolist(struct GlobalVariableWordValueMap *map, const struct CodeBlockOriginList *list) {
	if (list->origin_count) {
		int error_code;
		int i;
		if ((error_code = copy_gvwvmap(map, &list->sorted_origins[0]->var_values))) {
			return error_code;
		}

		for (i = 1; i < list->origin_count; i++) {
			if ((error_code = merge_gvwvmap(map, &list->sorted_origins[i]->var_values))) {
				return error_code;
			}
		}
	}

	return 0;
}

int index_of_first_cborigin_of_type_call_return(const struct CodeBlockOriginList *list) {
	int index;
	for (index = 0; index < list->origin_count; index++) {
		struct CodeBlockOrigin *origin_at_index = list->sorted_origins[index];
		unsigned int origin_type = get_cborigin_type(origin_at_index);
		if (origin_type > CBORIGIN_TYPE_CALL_RETURN) {
			return -1;
		}
		else if (origin_type == CBORIGIN_TYPE_CALL_RETURN) {
			return index;
		}
	}

	return -1;
}

int index_of_cborigin_of_type_continue(const struct CodeBlockOriginList *list) {
	int first = 0;
	int last = list->origin_count;
	while (last > first) {
		int index = (first + last) / 2;
		struct CodeBlockOrigin *origin_at_index = list->sorted_origins[index];
		unsigned int origin_type = get_cborigin_type(origin_at_index);
		if (origin_type < CBORIGIN_TYPE_CONTINUE) {
			first = index + 1;
		}
		else if (origin_type > CBORIGIN_TYPE_CONTINUE) {
			last = index;
		}
		else {
			return index;
		}
	}

	return -1;
}

int index_of_cborigin_of_type_call_return(const struct CodeBlockOriginList *list, unsigned int behind_count) {
	int first = 0;
	int last = list->origin_count;
	while (last > first) {
		int index = (first + last) / 2;
		struct CodeBlockOrigin *origin_at_index = list->sorted_origins[index];
		unsigned int origin_type = get_cborigin_type(origin_at_index);
		if (origin_type < CBORIGIN_TYPE_CALL_RETURN) {
			first = index + 1;
		}
		else if (origin_type > CBORIGIN_TYPE_CALL_RETURN) {
			last = index;
		}
		else {
			unsigned int origin_behind_count = get_cborigin_behind_count(origin_at_index);
			if (origin_behind_count < behind_count) {
				first = index + 1;
			}
			else if (origin_behind_count > behind_count) {
				last = index;
			}
			else {
				return index;
			}
		}
	}

	return -1;
}

int add_call_return_type_cborigin(struct CodeBlockOriginList *list, const struct Stack *stack, unsigned int behind_count) {
	if (index_of_cborigin_of_type_call_return(list, behind_count) < 0) {
		int error_code;
		struct CodeBlockOrigin *new_origin = prepare_new_cborigin(list);
		if (!new_origin) {
			return 1;
		}

		set_call_return_type_in_cborigin(new_origin, behind_count);
		make_all_registers_undefined(&new_origin->regs);
		initialize_stack(&new_origin->stack);
		if ((error_code = copy_stack(&new_origin->stack, stack))) {
			return error_code;
		}

		initialize_gvwvmap(&new_origin->var_values);
		if ((error_code = insert_cborigin(list, new_origin))) {
			return error_code;
		}
	}

	return 0;
}
