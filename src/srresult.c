#include "srresult.h"

#define SRRESULT_FLAG_DS_MATCHES_CS_AT_START 1

int ds_should_match_cs_at_segment_start(const struct SegmentReadResult *result) {
	return result->flags & SRRESULT_FLAG_DS_MATCHES_CS_AT_START;
}

void mark_ds_matches_cs_at_start(struct SegmentReadResult *result) {
	result->flags = SRRESULT_FLAG_DS_MATCHES_CS_AT_START;
}
