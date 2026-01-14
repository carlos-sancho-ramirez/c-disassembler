#ifndef _SEGMENT_READ_RESULT_H_
#define _SEGMENT_READ_RESULT_H_

#include "fpointer.h"

struct SegmentReadResult {
	struct FarPointer *relocation_table;
	unsigned int relocation_count;
	const char **sorted_relocations;
	char *buffer;
	unsigned int size;
	int relative_cs;
	unsigned int ip;
	unsigned int flags;
};

int ds_should_match_cs_at_segment_start(const struct SegmentReadResult *result);
void mark_ds_matches_cs_at_start(struct SegmentReadResult *result);

#endif /* _SEGMENT_READ_RESULT_H_ */
