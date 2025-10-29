#include "global_variables.h"

DEFINE_STRUCT_LIST_METHODS(GlobalVariable, global_variable, variable, start, 8, 256)

#define GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD (sizeof(unsigned int) * 8)
#define GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_GRANULARITY 4
#define GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY (GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_GRANULARITY * GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD)

void initialize_global_variable_word_value_map(struct GlobalVariableWordValueMap *map) {
	map->entry_count = 0;
	map->keys = NULL;
	map->values = NULL;
	map->relative = NULL;
}

int is_global_variable_word_value_relative_at_index(const struct GlobalVariableWordValueMap *map, int index) {
	return map->relative[index / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] & (1 << (index % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD));
}

uint16_t get_global_variable_word_value_at_index(const struct GlobalVariableWordValueMap *map, int index) {
	return map->values[index];
}

int index_of_global_variable_in_word_value_map_with_start(const struct GlobalVariableWordValueMap *map, const char *start) {
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

int put_global_variable_in_word_value_map(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value) {
	int first = 0;
	int last = map->entry_count;
	int i;

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
			map->keys[index] = key;
			map->values[index] = value;
			map->relative[index / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] &= ~(1 << (index % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD));
			return 0;
		}
	}

	if ((map->entry_count % GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY) == 0) {
		size_t new_size = map->entry_count + GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY;
		map->keys = realloc(map->keys, new_size);
		map->values = realloc(map->values, new_size);
		map->relative = realloc(map->relative, new_size / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);
		if (!map->keys || !map->values || !map->relative) {
			return 1;
		}
	}

	for (i = map->entry_count; i > last; i--) {
		const int target_index = i / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD;
		const int mask = 1 << (i % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);

		map->keys[i] = map->keys[i - 1];
		map->values[i] = map->values[i - 1];

		if (map->relative[(i - 1) / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] & (1 << ((i - 1) % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD))) {
			map->relative[target_index] |= mask;
		}
		else {
			map->relative[target_index] &= ~mask;
		}
	}

	map->keys[last] = key;
	map->values[last] = value;
	map->relative[last / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] &= ~(1 << (last % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD));
	map->entry_count++;

	return 0;
}

int put_global_variable_in_word_value_map_relative(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value) {
	int first = 0;
	int last = map->entry_count;
	int i;

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
			map->keys[index] = key;
			map->values[index] = value;
			map->relative[index / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] |= 1 << (index % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);
			return 0;
		}
	}

	if ((map->entry_count % GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY) == 0) {
		size_t new_size = map->entry_count + GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY;
		map->keys = realloc(map->keys, new_size);
		map->values = realloc(map->values, new_size);
		map->relative = realloc(map->relative, new_size / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);
		if (!map->keys || !map->values || !map->relative) {
			return 1;
		}
	}

	for (i = map->entry_count; i > last; i--) {
		const int target_index = i / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD;
		const int mask = 1 << (i % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);

		map->keys[i] = map->keys[i - 1];
		map->values[i] = map->values[i - 1];

		if (map->relative[(i - 1) / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] & (1 << ((i - 1) % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD))) {
			map->relative[target_index] |= mask;
		}
		else {
			map->relative[target_index] &= ~mask;
		}
	}

	map->keys[last] = key;
	map->values[last] = value;
	map->relative[last / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] |= 1 << (last % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);
	map->entry_count++;

	return 0;
}

int remove_global_variable_word_value_with_start(struct GlobalVariableWordValueMap *map, const char *key) {
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
				const int target_index = (i - 1) / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD;
				const int mask = 1 << ((i - 1) % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);

				map->keys[i - 1] = map->keys[i];
				map->values[i - 1] = map->values[i];

				if (map->relative[i / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD] & (1 << (i % GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD))) {
					map->relative[target_index] |= mask;
				}
				else {
					map->relative[target_index] &= ~mask;
				}
			}

			if ((--map->entry_count % GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY) == 0) {
				map->keys = realloc(map->keys, map->entry_count);
				map->values = realloc(map->values, map->entry_count);
				map->relative = realloc(map->relative, map->entry_count / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD);
				if (!map->keys || !map->values || !map->relative) {
					return 1;
				}
			}

			return 0;
		}
	}

	return 0;
}

void clear_global_variable_word_value_map(struct GlobalVariableWordValueMap *map) {
	if (map->keys) {
		free(map->keys);
	}

	if (map->values) {
		free(map->values);
	}

	if (map->relative) {
		free(map->relative);
	}

	initialize_global_variable_word_value_map(map);
}

int copy_global_variable_word_values_map(struct GlobalVariableWordValueMap *target_map, const struct GlobalVariableWordValueMap *source_map) {
	const unsigned int count = source_map->entry_count;
	const unsigned int allocated_count = ((count + GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY - 1) / GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY) * GLOBAL_VARIABLE_WORD_VALUE_MAP_ARRAY_GRANULARITY;

	if (target_map->keys) {
		free(target_map->keys);
	}

	if (target_map->values) {
		free(target_map->values);
	}

	if (target_map->relative) {
		free(target_map->relative);
	}

	if (allocated_count) {
		const int relative_allocated_count = allocated_count / GLOBAL_VARIABLE_WORD_VALUE_MAP_RELATIVE_BITS_PER_WORD;
		int i;
		target_map->keys = malloc(allocated_count * sizeof(const char *));
		target_map->values = malloc(allocated_count * sizeof(uint16_t));
		target_map->relative = malloc(relative_allocated_count * sizeof(unsigned int));
		if (!target_map->keys || !target_map->values || !target_map->relative) {
			return 1;
		}

		for (i = 0; i < count; i++) {
			target_map->keys[i] = source_map->keys[i];
			target_map->values[i] = source_map->values[i];
		}

		for (i = 0; i < count; i++) {
			target_map->relative[i] = source_map->relative[i];
		}
	}
	else {
		target_map->keys = NULL;
		target_map->values = NULL;
		target_map->relative = NULL;
	}
	target_map->entry_count = count;

	return 0;
}

int merge_global_variable_word_values_map(struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map) {
	const char *key;
	int index;
	int error_code;
	int i;
	for (i = 0; i < map->entry_count; i++) {
		key = map->keys[i];
		index = index_of_global_variable_in_word_value_map_with_start(other_map, key);
		if (index < 0 || map->values[i] != other_map->values[i] || is_global_variable_word_value_relative_at_index(map, i) != is_global_variable_word_value_relative_at_index(other_map, index)) {
			if ((error_code = remove_global_variable_word_value_with_start(map, key))) {
				return error_code;
			}
			--i;
		}
	}

	return 0;
}

int changes_on_merging_global_variable_word_values_map(const struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map) {
	const char *key;
	int index;
	int error_code;
	int i;
	for (i = 0; i < map->entry_count; i++) {
		key = map->keys[i];
		index = index_of_global_variable_in_word_value_map_with_start(other_map, key);
		if (index < 0 || map->values[i] != other_map->values[i] || is_global_variable_word_value_relative_at_index(map, i) != is_global_variable_word_value_relative_at_index(other_map, index)) {
			return 1;
		}
	}

	return 0;
}

#include "refs.h"

int add_global_variable_reference(
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list,
		struct Registers *regs,
		int segment_index,
		int result_address,
		const char *segment_start,
		const int value0,
		const char *opcode_reference) {
	if (is_segment_register_defined_and_relative(regs, segment_index)) {
		int error_code;
		unsigned int segment_value = get_segment_register(regs, segment_index);
		unsigned int relative_address = (segment_value * 16 + result_address) & 0xFFFF;
		const char *target = segment_start + relative_address;
		const int var_index = index_of_global_variable_with_start(global_variable_list, target);
		struct GlobalVariable *var;
		if (var_index >= 0) {
			var = global_variable_list->sorted_variables[var_index];
		}
		else {
			var = prepare_new_global_variable(global_variable_list);
			var->start = target;
			var->relative_address = relative_address;
			if (value0 & 1) {
				var->end = target + 2;
				var->var_type = GLOBAL_VARIABLE_TYPE_WORD;
			}
			else {
				var->end = target + 1;
				var->var_type = GLOBAL_VARIABLE_TYPE_BYTE;
			}

			if ((error_code = insert_sorted_global_variable(global_variable_list, var))) {
				return error_code;
			}
		}

		if (segment_value && segment_value != 0xFFF0) {
			const char *target_segment_start = segment_start + segment_value * 16;
			if (!contains_segment_start(segment_start_list, target_segment_start)) {
				if ((error_code = insert_segment_start(segment_start_list, target_segment_start))) {
					return error_code;
				}
			}
		}

		if (index_of_reference_with_instruction(reference_list, opcode_reference) < 0) {
			struct Reference *new_ref = prepare_new_reference(reference_list);
			new_ref->instruction = opcode_reference;
			set_global_variable_reference_from_instruction_address(new_ref, var);
			if ((error_code = insert_sorted_reference(reference_list, new_ref))) {
				return error_code;
			}
		}
	}

	return 0;
}

int add_far_pointer_global_variable_reference(
		struct GlobalVariableList *global_variable_list,
		struct ReferenceList *reference_list,
		struct Registers *regs,
		int segment_index,
		int result_address,
		const char *segment_start,
		const char *opcode_reference) {
	if (is_segment_register_defined_and_relative(regs, segment_index)) {
		int error_code;
		unsigned int relative_address = (get_segment_register(regs, segment_index) * 16 + result_address) & 0xFFFF;
		const char *target = segment_start + relative_address;
		const int var_index = index_of_global_variable_with_start(global_variable_list, target);
		struct GlobalVariable *var;
		if (var_index >= 0) {
			var = global_variable_list->sorted_variables[var_index];
			if (var->var_type == GLOBAL_VARIABLE_TYPE_WORD) {
				var->end += 2;
				var->var_type = GLOBAL_VARIABLE_TYPE_FAR_POINTER;
			}
		}
		else {
			var = prepare_new_global_variable(global_variable_list);
			var->start = target;
			var->relative_address = relative_address;
			var->end = target + 4;
			var->var_type = GLOBAL_VARIABLE_TYPE_FAR_POINTER;

			if ((error_code = insert_sorted_global_variable(global_variable_list, var))) {
				return error_code;
			}
		}

		if (index_of_reference_with_instruction(reference_list, opcode_reference) < 0) {
			struct Reference *new_ref = prepare_new_reference(reference_list);
			new_ref->instruction = opcode_reference;
			set_global_variable_reference_from_instruction_address(new_ref, var);
			if ((error_code = insert_sorted_reference(reference_list, new_ref))) {
				return error_code;
			}
		}
	}

	return 0;
}
