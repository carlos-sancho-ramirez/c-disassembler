#include "funclist.h"
#include "printd.h"

static void log_func_insertion(struct Function *func) {
#ifdef DEBUG
	const unsigned int starting_block_count = get_starting_block_count(func);
	DEBUG_PRINT1("  Registering new function at +%x:", func->blocks[0]->relative_cs);
	if (starting_block_count == 1) {
		const struct CodeBlock *start_block = get_starting_block(func, 0);
		DEBUG_PRINT1("%x\n", start_block->ip);
	}
	else {
		unsigned int i;
		DEBUG_PRINT1("{%x", get_starting_block(func, 0)->ip);
		for (i = 1; i < starting_block_count; i++) {
			DEBUG_PRINT1(", %x", get_starting_block(func, i)->ip);
		}

		DEBUG_PRINT0("}\n");
	}
#endif /* DEBUG */
}

DEFINE_STRUCT_LIST_INITIALIZE_METHOD(Function, func, func)
DEFINE_STRUCT_LIST_GET_UNSORTED_METHOD(Function, func, 64)
DEFINE_STRUCT_LIST_PREPARE_NEW_METHOD(Function, func, func, 8, 64)

void clear_func_list(struct FunctionList *list) {
	if (list->func_count > 0) {
		const int allocated_pages = (list->func_count + 64 - 1) / 64;
		int entries_in_last_page = list->func_count & 0x3F;
		int i;

		for (i = allocated_pages - 1; i >= 0; i--) {
			struct Function *page = list->page_array[i];
			const int last_entry_in_this_page = (i < allocated_pages - 1 || entries_in_last_page == 0)? 63 : entries_in_last_page - 1;
			int j;

			for (j = last_entry_in_this_page; j >= 0; j--) {
				free_func_content(page + j);
			}

			free(page);
		}

		free(list->page_array);
		free(list->sorted_funcs);
		list->page_array = NULL;
		list->sorted_funcs = NULL;
		list->func_count = 0;
	}
}

int index_of_func_containing_block_start(const struct FunctionList *list, const char *start) {
	int first = 0;
	int last = list->func_count;

	while (last > first) {
		int index = (first + last) / 2;
		struct Function *this_func = list->sorted_funcs[index];
		struct CodeBlock *first_block = this_func->blocks[0];
		const char *first_block_start = first_block->start;
		if (start < first_block_start) {
			last = index;
		}
		else if (start == first_block_start) {
			return index;
		}
		else {
			struct CodeBlock *last_block = this_func->blocks[this_func->block_count - 1];
			const char *last_block_start = last_block->start;

			if (start > last_block_start) {
				first = index + 1;
			}
			else if (start == last_block_start) {
				return index;
			}
			else {
				int first_block_index = 1;
				int last_block_index = this_func->block_count - 1;
				while (last_block_index > first_block_index) {
					const int block_index = (first_block_index + last_block_index) / 2;
					struct CodeBlock *this_block = this_func->blocks[block_index];
					if (this_block->start < start) {
						first_block_index = block_index + 1;
					}
					else if (this_block->start > start) {
						last_block_index = block_index;
					}
					else {
						return index;
					}
				}

				first = index + 1;
			}
		}
	}

	return -1;
}

int insert_func(struct FunctionList *list, struct Function *new_func) {
	int first = 0;
	int last = list->func_count;
	int i;
	const char *new_func_first_block_start = new_func->blocks[0]->start;
	log_func_insertion(new_func);
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->sorted_funcs[index]->blocks[0]->start;
		if (this_start < new_func_first_block_start) {
			first = index + 1;
		}
		else if (this_start > new_func_first_block_start) {
			last = index;
		}
		else {
			return -1;
		}
	}

	for (i = list->func_count; i > last; i--) {
		list->sorted_funcs[i] = list->sorted_funcs[i - 1];
	}

	list->sorted_funcs[last] = new_func;
	list->func_count++;

	return 0;
}

#ifdef DEBUG

#include <stdio.h>

void print_funclist(const struct FunctionList *list) {
	int i;
	fprintf(stderr, "FunctionList(\n ");
	for (i = 0; i < list->func_count; i++) {
		print_func(list->sorted_funcs[i]);
		if (i + 1 == list->func_count) {
			fprintf(stderr, ")");
		}
		fprintf(stderr, "\n ");
	}
}

#endif /* DEBUG */
