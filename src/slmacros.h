#ifndef _STRUCT_LIST_MACROS_H_
#define _STRUCT_LIST_MACROS_H_

#include <stdlib.h>

#define DEFINE_STRUCT_LIST(struct_name, short_item_name) \
/** \
 * Complex structure containing pages of the given struct in struct_name. \
 * It is designed to grow as more references are added to it. \
 */ \
struct struct_name ## List { \
	/** \
	 * Array holding all allocated pages in the order they have been allocated. \
	 * The allocated size of this array can be calculated combining the values in short_item_name##_count, \
	 * short_item_name##s_per_page and page_array_granularity. \
	 * This will be NULL when short_item_name##_count is 0. \
	 */ \
	struct struct_name **page_array; \
	\
	/** \
	 * Array pointing to all structs, sorted by its pointer. \
	 * The allocated size of this array can be calculated combining the values in short_item_name##_count, \
	 * short_item_name##s_per_page and page_array_granularity. \
	 * This will be NULL when reference_count is 0. \
	 */ \
	struct struct_name **sorted_##short_item_name##s; \
	\
	/** \
	 * Number of structs already inserted. \
	 * Note that this value is most of the times lower than the actual capacity allocated in memory \
	 * to hold all of them. \
	 */ \
	unsigned int short_item_name##_count; \
}

#define DECLARE_STRUCT_LIST_INITIALIZE_METHOD(struct_name, struct_name_snake) \
/** \
 * Set all its values. After this, this list will be empty, but ready. \
 */ \
void initialize_##struct_name_snake##_list(struct struct_name##List *list)

#define DECLARE_STRUCT_LIST_GET_UNSORTED_METHOD(struct_name, struct_name_snake) \
struct struct_name *get_unsorted_##struct_name_snake(const struct struct_name##List *list, int index)

#define DECLARE_STRUCT_LIST_PREPARE_NEW_METHOD(struct_name, struct_name_snake) \
/** \
 * Returns a proper pointer to store a new short_item_name. \
 * The returned pointer should be filled and call insert_##struct_name_snake method in order to sort it properly. \
 * This method may require allocating a new page of memory. \
 * This method will return NULL in case of failure. \
 */ \
struct struct_name *prepare_new_##struct_name_snake(struct struct_name##List *list) \

#define DECLARE_STRUCT_LIST_CLEAR_METHOD(struct_name, struct_name_snake) \
/** \
 * Free all the allocated short_item_name and restores this list to its initial state. \
 */ \
void clear_##struct_name_snake##_list(struct struct_name##List *list)

#define DECLARE_STRUCT_LIST_METHODS(struct_name, struct_name_snake, short_item_name, sorted_property) \
DECLARE_STRUCT_LIST_INITIALIZE_METHOD(struct_name, struct_name_snake); \
DECLARE_STRUCT_LIST_GET_UNSORTED_METHOD(struct_name, struct_name_snake); \
DECLARE_STRUCT_LIST_PREPARE_NEW_METHOD(struct_name, struct_name_snake); \
DECLARE_STRUCT_LIST_CLEAR_METHOD(struct_name, struct_name_snake); \
\
struct struct_name *get_unsorted_##struct_name_snake(const struct struct_name##List *list, int index); \
\
/** \
 * Searches for a short_item_name whose sorted_property matches the given one. \
 * This will return the index of the short_item_name within the sorted_##short_item_name##s array. Or -1 if none matches. \
 */ \
int index_of_##struct_name_snake##_with_##sorted_property(const struct struct_name##List *list, const char *sorted_property); \
\
/** \
 * Searches for a short_item_name whose sorted_property matches the given position. \
 * If none matches, this method will return the short_item_name whose start is closest but before the given position. \
 * This will return -1 if there are not short_item_name##s, or all of them has a sorted_property greater than the given position. \
 * This will return the index of the short_item_name within the sorted_##short_item_name##s array. \
 */ \
int index_of_##struct_name_snake##_containing_position(const struct struct_name##List *list, const char *position); \
\
/** \
 * Inserts the short_item_name returned previously by prepare_new_##struct_name_snake. \
 * This will increase the short_item_name##_count value by 1 and will update the sorted_#short_item_name##s inserting the \
 * reference to this short_item_name in its suitable position. \
 */ \
int insert_##struct_name_snake(struct struct_name##List *list, struct struct_name *new_##short_item_name)

#define DEFINE_STRUCT_LIST_INITIALIZE_METHOD(struct_name, struct_name_snake, short_item_name) \
void initialize_##struct_name_snake##_list(struct struct_name##List *list) { \
	list->short_item_name##_count = 0; \
	list->page_array = NULL; \
	list->sorted_##short_item_name##s = NULL; \
}

#define DEFINE_STRUCT_LIST_GET_UNSORTED_METHOD(struct_name, struct_name_snake, initial_items_per_page) \
struct struct_name *get_unsorted_##struct_name_snake(const struct struct_name##List *list, int index) { \
	return list->page_array[index / initial_items_per_page] + (index % initial_items_per_page); \
}

#define DEFINE_STRUCT_LIST_PREPARE_NEW_METHOD(struct_name, struct_name_snake, short_item_name, initial_page_array_granularity, initial_items_per_page) \
struct struct_name *prepare_new_##struct_name_snake(struct struct_name##List *list) { \
	if ((list->short_item_name##_count % initial_items_per_page) == 0) { \
		struct struct_name *new_page; \
		if ((list->short_item_name##_count % (initial_items_per_page * initial_page_array_granularity)) == 0) { \
			const int new_page_array_length = (list->short_item_name##_count / (initial_items_per_page * initial_page_array_granularity)) + initial_page_array_granularity; \
			list->page_array = realloc(list->page_array, new_page_array_length * sizeof(struct struct_name *)); \
			if (!(list->page_array)) { \
				return NULL; \
			} \
\
			list->sorted_##short_item_name##s = realloc(list->sorted_##short_item_name##s, new_page_array_length * initial_items_per_page * sizeof(struct struct_name *)); \
			if (!(list->sorted_##short_item_name##s)) { \
				return NULL; \
			} \
		} \
\
		new_page = malloc(initial_items_per_page * sizeof(struct struct_name)); \
		if (!new_page) { \
			return NULL; \
		} \
\
		list->page_array[list->short_item_name##_count / initial_items_per_page] = new_page; \
	} \
\
	return list->page_array[list->short_item_name##_count / initial_items_per_page] + (list->short_item_name##_count % initial_items_per_page); \
}

#define DEFINE_STRUCT_LIST_CLEAR_METHOD(struct_name, struct_name_snake, short_item_name, initial_items_per_page) \
void clear_##struct_name_snake##_list(struct struct_name##List *list) { \
	if (list->short_item_name##_count > 0) { \
		const int allocated_pages = (list->short_item_name##_count + initial_items_per_page - 1) / initial_items_per_page; \
		int i; \
		for (i = allocated_pages - 1; i >= 0; i--) { \
			free(list->page_array[i]); \
		} \
\
		free(list->page_array); \
		free(list->sorted_##short_item_name##s); \
		list->page_array = NULL; \
		list->sorted_##short_item_name##s = NULL; \
		list->short_item_name##_count = 0; \
	} \
}

#define DEFINE_STRUCT_LIST_METHODS(struct_name, struct_name_snake, short_item_name, sorted_property, initial_page_array_granularity, initial_items_per_page) \
DEFINE_STRUCT_LIST_INITIALIZE_METHOD(struct_name, struct_name_snake, short_item_name) \
DEFINE_STRUCT_LIST_GET_UNSORTED_METHOD(struct_name, struct_name_snake, initial_items_per_page) \
DEFINE_STRUCT_LIST_PREPARE_NEW_METHOD(struct_name, struct_name_snake, short_item_name, initial_page_array_granularity, initial_items_per_page) \
DEFINE_STRUCT_LIST_CLEAR_METHOD(struct_name, struct_name_snake, short_item_name, initial_items_per_page) \
\
int index_of_##struct_name_snake##_with_##sorted_property(const struct struct_name##List *list, const char *sorted_property) { \
	int first = 0; \
	int last = list->short_item_name##_count; \
	while (last > first) { \
		int index = (first + last) / 2; \
		const char *this_##sorted_property = list->sorted_##short_item_name##s[index]->sorted_property; \
		if (this_##sorted_property < sorted_property) { \
			first = index + 1; \
		} \
		else if (this_##sorted_property > sorted_property) { \
			last = index; \
		} \
		else { \
			return index; \
		} \
	} \
\
	return -1; \
} \
\
int index_of_##struct_name_snake##_containing_position(const struct struct_name##List *list, const char *position) { \
	int first = 0; \
	int last = list->short_item_name##_count; \
	while (last > first) { \
		int index = (first + last) / 2; \
		const char *this_##sorted_property = list->sorted_##short_item_name##s[index]->sorted_property; \
		if (this_##sorted_property < position) { \
			first = index + 1; \
		} \
		else if (this_##sorted_property > position) { \
			last = index; \
		} \
		else { \
			return index; \
		} \
	} \
\
	return first - 1; \
} \
\
int insert_##struct_name_snake(struct struct_name##List *list, struct struct_name *new_##short_item_name) { \
	int first = 0; \
	int last = list->short_item_name##_count; \
	int i; \
	while (last > first) { \
		int index = (first + last) / 2; \
		const char *this_##sorted_property = list->sorted_##short_item_name##s[index]->sorted_property; \
		if (this_##sorted_property < new_##short_item_name->sorted_property) { \
			first = index + 1; \
		} \
		else if (this_##sorted_property > new_##short_item_name->sorted_property) { \
			last = index; \
		} \
		else { \
			return -1; \
		} \
	} \
\
	for (i = list->short_item_name##_count; i > last; i--) { \
		list->sorted_##short_item_name##s[i] = list->sorted_##short_item_name##s[i - 1]; \
	} \
\
	list->sorted_##short_item_name##s[last] = new_##short_item_name; \
	list->short_item_name##_count++; \
\
	return 0; \
}

#endif
