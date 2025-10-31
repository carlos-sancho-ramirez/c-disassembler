#ifndef _GLOBAL_VARIABLE_WORD_VALUE_MAP_H_
#define _GLOBAL_VARIABLE_WORD_VALUE_MAP_H_

#include <stdint.h>

struct GlobalVariableWordValueMap {
		const char **keys;
		uint16_t *values;
		unsigned int *relative;
		unsigned int entry_count;
};

void initialize_global_variable_word_value_map(struct GlobalVariableWordValueMap *map);
int is_global_variable_word_value_relative_at_index(const struct GlobalVariableWordValueMap *map, int index);
uint16_t get_global_variable_word_value_at_index(const struct GlobalVariableWordValueMap *map, int index);
int index_of_global_variable_in_word_value_map_with_start(const struct GlobalVariableWordValueMap *map, const char *start);
int put_global_variable_in_word_value_map(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value);
int put_global_variable_in_word_value_map_relative(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value);
int remove_global_variable_word_value_with_start(struct GlobalVariableWordValueMap *map, const char *key);
void clear_global_variable_word_value_map(struct GlobalVariableWordValueMap *map);

int copy_global_variable_word_values_map(struct GlobalVariableWordValueMap *target_map, const struct GlobalVariableWordValueMap *source_map);
int merge_global_variable_word_values_map(struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map);
int changes_on_merging_global_variable_word_values_map(const struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map);

#endif /* _GLOBAL_VARIABLE_WORD_VALUE_MAP_H_ */
