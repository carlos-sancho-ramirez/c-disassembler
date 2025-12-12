#include "gvwvmap.h"
#include <stdlib.h>

#define GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD (sizeof(uint16_t) * 4)
#define GVWVMAP_DEFINED_RELATIVE_GRANULARITY 4
#define GVWVMAP_ARRAY_WORD_GRANULARITY (GVWVMAP_DEFINED_RELATIVE_GRANULARITY * GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD)

void initialize_gvwvmap(struct GlobalVariableWordValueMap *map) {
	map->entry_count = 0;
	map->keys = NULL;
	map->values = NULL;
	map->defined_and_relative = NULL;
}

int is_gvwvalue_defined_at_index(const struct GlobalVariableWordValueMap *map, int index) {
	const unsigned int shift = (index % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;
	const uint16_t mask = 3 << shift;
	return (map->defined_and_relative[index / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD] & (3 << shift)) == (1 << shift);
}

int is_gvwvalue_defined_relative_at_index(const struct GlobalVariableWordValueMap *map, int index) {
	const uint16_t mask = 3 << ((index % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2);
	return (map->defined_and_relative[index / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD] & mask) == mask;
}

uint16_t get_gvwvalue_at_index(const struct GlobalVariableWordValueMap *map, int index) {
	return map->values[index];
}

int index_of_gvar_in_gvwvmap_with_start(const struct GlobalVariableWordValueMap *map, const char *start) {
	int first = 0;
	int last = map->entry_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = map->keys[index];
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

static int enlarge_one_page_if_required(struct GlobalVariableWordValueMap *map) {
	if ((map->entry_count % GVWVMAP_ARRAY_WORD_GRANULARITY) == 0) {
		const unsigned int new_allocated_word_count = map->entry_count + GVWVMAP_ARRAY_WORD_GRANULARITY;
		size_t new_size = map->entry_count + GVWVMAP_ARRAY_WORD_GRANULARITY * sizeof(uint16_t);
		map->keys = realloc(map->keys, new_allocated_word_count * sizeof(const char *));
		map->values = realloc(map->values, new_allocated_word_count * sizeof(uint16_t));
		map->defined_and_relative = realloc(map->defined_and_relative, (new_allocated_word_count / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * sizeof(uint16_t));
		if (!map->keys || !map->values || !map->defined_and_relative) {
			return 1;
		}
	}

	return 0;
}

static void ensure_gap(struct GlobalVariableWordValueMap *map, int gap_index) {
	int i;

	for (i = map->entry_count; i > gap_index; i--) {
		const int source_index = (i - 1) / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
		const int target_index = i / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
		const int source_shift = ((i - 1) % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;
		const int target_shift = (i % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;
		const uint16_t defined_value = 1 << target_shift;

		map->keys[i] = map->keys[i - 1];
		map->values[i] = map->values[i - 1];

		if (map->defined_and_relative[source_index] & (1 << source_shift)) {
			const uint16_t relative_value = 2 << target_shift;
			map->defined_and_relative[target_index] |= defined_value;

			if (map->defined_and_relative[source_index] & (2 << source_shift)) {
				map->defined_and_relative[target_index] |= relative_value;
			}
			else {
				map->defined_and_relative[target_index] &= ~relative_value;
			}
		}
		else {
			map->defined_and_relative[target_index] &= ~defined_value;
		}
	}
}

int put_gvar_in_gvwvmap(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value) {
	int first = 0;
	int last = map->entry_count;

	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = map->keys[index];
		if (this_start < key) {
			first = index + 1;
		}
		else if (this_start > key) {
			last = index;
		}
		else {
			const unsigned int word_index = index / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
			const unsigned int shift = (index % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;
			map->keys[index] = key;
			map->values[index] = value;
			map->defined_and_relative[word_index] |= 1 << shift;
			map->defined_and_relative[word_index] &= ~(2 << shift);
			return 0;
		}
	}

	if (enlarge_one_page_if_required(map)) {
		return 1;
	}

	ensure_gap(map, last);
	map->keys[last] = key;
	map->values[last] = value;
	map->defined_and_relative[last / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD] |= 1 << ((last % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2);
	map->defined_and_relative[last / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD] &= ~(2 << ((last % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2));
	map->entry_count++;

	return 0;
}

int put_gvar_in_gvwvmap_relative(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value) {
	int first = 0;
	int last = map->entry_count;

	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = map->keys[index];
		if (this_start < key) {
			first = index + 1;
		}
		else if (this_start > key) {
			last = index;
		}
		else {
			const unsigned int word_index = index / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
			const unsigned int shift = (index % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;
			map->keys[index] = key;
			map->values[index] = value;
			map->defined_and_relative[word_index] |= 3 << shift;
			return 0;
		}
	}

	if (enlarge_one_page_if_required(map)) {
		return 1;
	}

	ensure_gap(map, last);
	map->keys[last] = key;
	map->values[last] = value;
	map->defined_and_relative[last / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD] |= 3 << ((last % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2);
	map->entry_count++;

	return 0;
}

int put_gvar_in_gvwvmap_undefined(struct GlobalVariableWordValueMap *map, const char *key) {
	int first = 0;
	int last = map->entry_count;

	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = map->keys[index];
		if (this_start < key) {
			first = index + 1;
		}
		else if (this_start > key) {
			last = index;
		}
		else {
			const unsigned int word_index = index / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
			const unsigned int shift = (index % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;
			map->keys[index] = key;
			map->defined_and_relative[word_index] &= ~(3 << shift);
			return 0;
		}
	}

	if (enlarge_one_page_if_required(map)) {
		return 1;
	}

	ensure_gap(map, last);
	map->keys[last] = key;
	map->defined_and_relative[last / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD] &= ~(3 << ((last % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2));
	map->entry_count++;

	return 0;
}

int remove_gvwvalue_with_start(struct GlobalVariableWordValueMap *map, const char *key) {
	int first = 0;
	int last = map->entry_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = map->keys[index];
		if (this_start < key) {
			first = index + 1;
		}
		else if (this_start > key) {
			last = index;
		}
		else {
			int i;
			for (i = index + 1; i < map->entry_count; i++) {
				const int source_index = i / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
				const int target_index = (i - 1) / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
				const int source_shift = (i % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;
				const int target_shift = ((i - 1) % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2;

				const int mask = 1 << ((i - 1) % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD);

				map->keys[i - 1] = map->keys[i];
				map->values[i - 1] = map->values[i];

				if (map->defined_and_relative[source_index] & (1 << source_shift)) {
					if (map->defined_and_relative[source_index] & (2 << source_shift)) {
						map->defined_and_relative[target_index] |= 3 << target_shift;
					}
					else {
						map->defined_and_relative[target_index] |= 1 << target_shift;
						map->defined_and_relative[target_index] &= ~(2 << target_shift);
					}
				}
				else {
					map->defined_and_relative[target_index] &= ~(3 << target_shift);
				}
			}

			if ((--map->entry_count % GVWVMAP_ARRAY_WORD_GRANULARITY) == 0) {
				map->keys = realloc(map->keys, map->entry_count * sizeof(const char *));
				map->values = realloc(map->values, map->entry_count * sizeof(uint16_t));
				map->defined_and_relative = realloc(map->defined_and_relative, (map->entry_count / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * sizeof(uint16_t));
				if (!map->keys || !map->values || !map->defined_and_relative) {
					return 1;
				}
			}

			return 0;
		}
	}

	return 0;
}

void clear_gvwvmap(struct GlobalVariableWordValueMap *map) {
	if (map->keys) {
		free(map->keys);
	}

	if (map->values) {
		free(map->values);
	}

	if (map->defined_and_relative) {
		free(map->defined_and_relative);
	}

	initialize_gvwvmap(map);
}

int copy_gvwvmap(struct GlobalVariableWordValueMap *target_map, const struct GlobalVariableWordValueMap *source_map) {
	const unsigned int old_count = target_map->entry_count;
	const unsigned int old_allocated_pages = (old_count + GVWVMAP_ARRAY_WORD_GRANULARITY - 1) / GVWVMAP_ARRAY_WORD_GRANULARITY;
	const unsigned int new_count = source_map->entry_count;
	const unsigned int new_allocated_pages = (new_count + GVWVMAP_ARRAY_WORD_GRANULARITY - 1) / GVWVMAP_ARRAY_WORD_GRANULARITY;
	const unsigned int new_allocated_count = new_allocated_pages * GVWVMAP_ARRAY_WORD_GRANULARITY;

	if (old_allocated_pages && old_allocated_pages != new_allocated_pages) {
		free(target_map->keys);
		free(target_map->values);
		free(target_map->defined_and_relative);
	}

	if (new_allocated_pages && old_allocated_pages != new_allocated_pages) {
		target_map->keys = malloc(new_allocated_count * sizeof(const char *));
		target_map->values = malloc(new_allocated_count * sizeof(uint16_t));
		target_map->defined_and_relative = malloc(new_allocated_count / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD * sizeof(uint16_t));
		if (!target_map->keys || !target_map->values || !target_map->defined_and_relative) {
			return 1;
		}
	}

	if (new_allocated_count) {
		const int relative_allocated_count = new_allocated_count / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD;
		int i;

		for (i = 0; i < new_count; i++) {
			target_map->keys[i] = source_map->keys[i];
			target_map->values[i] = source_map->values[i];
		}

		for (i = 0; i < relative_allocated_count; i++) {
			target_map->defined_and_relative[i] = source_map->defined_and_relative[i];
		}
	}
	else {
		target_map->keys = NULL;
		target_map->values = NULL;
		target_map->defined_and_relative = NULL;
	}
	target_map->entry_count = new_count;

	return 0;
}

int merge_gvwvmap(struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map) {
	int i;
	for (i = 0; i < map->entry_count; i++) {
		if (is_gvwvalue_defined_at_index(map, i)) {
			const char *key = map->keys[i];
			int other_index = index_of_gvar_in_gvwvmap_with_start(other_map, key);
			if (other_index < 0 || !is_gvwvalue_defined_at_index(other_map, other_index) || map->values[i] != other_map->values[i] || is_gvwvalue_defined_relative_at_index(map, i) != is_gvwvalue_defined_relative_at_index(other_map, other_index)) {
				map->defined_and_relative[i / GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD] &= ~(3 << ((i % GVWVMAP_DEFINED_RELATIVE_ENTRIES_PER_WORD) * 2));
			}
		}
	}

	return 0;
}

int changes_on_merging_gvwvmap(const struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map) {
	int i;
	for (i = 0; i < map->entry_count; i++) {
		if (is_gvwvalue_defined_at_index(map, i)) {
			const char *key = map->keys[i];
			int other_index = index_of_gvar_in_gvwvmap_with_start(other_map, key);
			if (other_index < 0 || !is_gvwvalue_defined_at_index(other_map, other_index) || map->values[i] != other_map->values[i] || is_gvwvalue_defined_relative_at_index(map, i) != is_gvwvalue_defined_relative_at_index(other_map, other_index)) {
				return 1;
			}
		}
	}

	return 0;
}

#ifdef DEBUG

#include <stdio.h>

void print_gvwvmap(const struct GlobalVariableWordValueMap *map, const char *buffer) {
	int i;
	fprintf(stderr, "Vars(");
	for (i = 0; i < map->entry_count; i++) {
		if (i > 0) {
			fprintf(stderr, ", ");
		}

		fprintf(stderr, "%lx->", map->keys[i] - buffer);
		if (is_gvwvalue_defined_at_index(map, i)) {
			if (is_gvwvalue_defined_relative_at_index(map, i)) {
				fprintf(stderr, "+");
			}

			fprintf(stderr, "%x", map->values[i]);
		}
		else {
			fprintf(stderr, "?");
		}
	}

	fprintf(stderr, ")");
}

#endif /* DEBUG */
