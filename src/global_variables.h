#ifndef _GLOBAL_VARIABLES_H_
#define _GLOBAL_VARIABLES_H_

#include <stdint.h>
#include <stdlib.h>
#include "struct_list_macros.h"

#define GLOBAL_VARIABLE_TYPE_BYTE 1
#define GLOBAL_VARIABLE_TYPE_WORD 2

/* Raw array of characters without any charater as end */
#define GLOBAL_VARIABLE_TYPE_STRING 5
#define GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING 6
#define GLOBAL_VARIABLE_TYPE_FAR_POINTER 9

struct GlobalVariable {
	const char *start;
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
};

DEFINE_STRUCT_LIST(GlobalVariable, variable);
DECLARE_STRUCT_LIST_METHODS(GlobalVariable, global_variable, variable, start);

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

#include "refs.h"
#include "registers.h"
#include "segments.h"

int add_global_variable_reference(
		struct GlobalVariableList *global_variable_list,
		struct SegmentStartList *segment_start_list,
		struct ReferenceList *reference_list,
		struct Registers *regs,
		int segment_index,
		int result_address,
		const char *segment_start,
		const int value0,
		const char *opcode_reference);

int add_far_pointer_global_variable_reference(
		struct GlobalVariableList *global_variable_list,
		struct ReferenceList *reference_list,
		struct Registers *regs,
		int segment_index,
		int result_address,
		const char *segment_start,
		const char *opcode_reference);

#endif
