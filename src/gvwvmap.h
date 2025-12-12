#ifndef _GLOBAL_VARIABLE_WORD_VALUE_MAP_H_
#define _GLOBAL_VARIABLE_WORD_VALUE_MAP_H_

#include <stdint.h>

struct GlobalVariableWordValueMap {
		const char **keys;
		uint16_t *values;
		uint16_t *defined_and_relative;
		unsigned int entry_count;
};

void initialize_gvwvmap(struct GlobalVariableWordValueMap *map);
int is_gvwvalue_defined_at_index(const struct GlobalVariableWordValueMap *map, int index);
int is_gvwvalue_defined_relative_at_index(const struct GlobalVariableWordValueMap *map, int index);
uint16_t get_gvwvalue_at_index(const struct GlobalVariableWordValueMap *map, int index);
int index_of_gvar_in_gvwvmap_with_start(const struct GlobalVariableWordValueMap *map, const char *start);
int put_gvar_in_gvwvmap(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value);
int put_gvar_in_gvwvmap_relative(struct GlobalVariableWordValueMap *map, const char *key, uint16_t value);
int put_gvar_in_gvwvmap_undefined(struct GlobalVariableWordValueMap *map, const char *key);
int remove_gvwvalue_with_start(struct GlobalVariableWordValueMap *map, const char *key);
void clear_gvwvmap(struct GlobalVariableWordValueMap *map);

int copy_gvwvmap(struct GlobalVariableWordValueMap *target_map, const struct GlobalVariableWordValueMap *source_map);
int merge_gvwvmap(struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map);
int changes_on_merging_gvwvmap(const struct GlobalVariableWordValueMap *map, const struct GlobalVariableWordValueMap *other_map);

#ifdef DEBUG
void print_gvwvmap(const struct GlobalVariableWordValueMap *map, const char *buffer);
#endif /* DEBUG */

#endif /* _GLOBAL_VARIABLE_WORD_VALUE_MAP_H_ */
