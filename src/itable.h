#ifndef _INTERRUPTION_TABLE_H_
#define _INTERRUPTION_TABLE_H_
#include "fpointer.h"

struct InterruptionTable {
	struct FarPointer pointers[256];

	/**
	 * Points to the last instruction where the offset was assigned to a register.
	 * This is irrelevant if the segment if not defined.
	 */
	const char *offset_origin[256];

	/**
	 * Points to the last instruction where the segment was assigned to a register.
	 * This is irrelevant if the segment if not defined.
	 */
	const char *segment_origin[256];

	uint16_t segment_defined[16];
	uint16_t offset_defined[16];
	uint16_t relative[16];
};

int is_interruption_defined_and_relative_in_table(struct InterruptionTable *table, uint8_t index);
const char *where_interruption_offset_defined_in_table(struct InterruptionTable *table, uint8_t index);
const char *where_interruption_segment_defined_in_table(struct InterruptionTable *table, uint8_t index);

uint16_t get_interruption_table_offset(struct InterruptionTable *table, uint8_t index);
uint16_t get_interruption_table_relative_segment(struct InterruptionTable *table, uint8_t index);

void set_interruption_table_offset(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value);
void set_interruption_table_segment(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value);
void set_interruption_table_segment_relative(struct InterruptionTable *table, uint8_t index, const char *where, uint16_t value);
void set_all_interruption_table_undefined(struct InterruptionTable *table);

#endif /* _INTERRUPTION_TABLE_H_ */
