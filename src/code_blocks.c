#include "code_blocks.h"

void initialize_code_block_list(struct CodeBlockList *list) {
	list->page_array_granularity = 8;
	list->blocks_per_page = 64;
	list->block_count = 0;
	list->page_array = NULL;
	list->sorted_blocks = NULL;
}

int index_of_code_block_with_start(const struct CodeBlockList *list, const char *start) {
	int first = 0;
	int last = list->block_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->sorted_blocks[index]->start;
		if (this_start < start) {
			first = index + 1;
		}
		else if (this_start > start) {
			last = index;
		}
		else {
			return index;
		}
	}

	return -1;
}

struct CodeBlock *prepare_new_code_block(struct CodeBlockList *list) {
	if ((list->block_count % list->blocks_per_page) == 0) {
		if ((list->block_count % (list->blocks_per_page * list->page_array_granularity)) == 0) {
			const int new_page_array_length = (list->block_count / (list->blocks_per_page * list->page_array_granularity)) + list->page_array_granularity;
			list->page_array = realloc(list->page_array, new_page_array_length * sizeof(struct CodeBlock *));
			if (!(list->page_array)) {
				return NULL;
			}

			list->sorted_blocks = realloc(list->sorted_blocks, new_page_array_length * list->blocks_per_page * sizeof(struct CodeBlock *));
			if (!(list->sorted_blocks)) {
				return NULL;
			}
		}

		struct CodeBlock *new_page = malloc(list->blocks_per_page * sizeof(struct CodeBlock));
		if (!new_page) {
			return NULL;
		}

		list->page_array[list->block_count / list->blocks_per_page] = new_page;
	}

	return list->page_array[list->block_count / list->blocks_per_page] + (list->block_count % list->blocks_per_page);
}

int insert_sorted_code_block(struct CodeBlockList *list, struct CodeBlock *new_block) {
	int first = 0;
	int last = list->block_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->sorted_blocks[index]->start;
		if (this_start < new_block->start) {
			first = index + 1;
		}
		else if (this_start > new_block->start) {
			last = index;
		}
		else {
			return -1;
		}
	}

	for (int i = list->block_count; i > last; i--) {
		list->sorted_blocks[i] = list->sorted_blocks[i - 1];
	}

	list->sorted_blocks[last] = new_block;
	list->block_count++;

	return 0;
}

void clear_code_block_list(struct CodeBlockList *list) {
	if (list->block_count > 0) {
		const int allocated_pages = (list->block_count + list->blocks_per_page - 1) / list->blocks_per_page;
		for (int i = allocated_pages - 1; i >= 0; i--) {
			free(list->page_array[i]);
		}

		free(list->page_array);
		free(list->sorted_blocks);
		list->page_array = NULL;
		list->sorted_blocks = NULL;
		list->block_count = 0;
	}
}
