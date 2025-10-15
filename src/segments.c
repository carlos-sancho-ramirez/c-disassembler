#include "segments.h"
#include <stdlib.h>

#define SEGMENT_START_LIST_GRANULARITY 8

void initialize_segment_start_list(struct SegmentStartList *list) {
    list->count = 0;
    list->start = NULL;
}

int contains_segment_start(struct SegmentStartList *list, const char *start) {
    int first = 0;
	int last = list->count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->start[index];
		if (this_start < start) {
			first = index + 1;
		}
		else if (this_start > start) {
			last = index;
		}
		else {
			return 1;
		}
	}

    return 0;
}

int insert_segment_start(struct SegmentStartList *list, const char *new_start) {
    if ((list->count % SEGMENT_START_LIST_GRANULARITY) == 0) {
        list->start = realloc(list->start, list->count + SEGMENT_START_LIST_GRANULARITY);
        if (!list->start) {
            return 1;
        }
    }

    int first = 0;
	int last = list->count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_start = list->start[index];
		if (this_start < new_start) {
			first = index + 1;
		}
		else if (this_start > new_start) {
			last = index;
		}
		else {
			return 1;
		}
	}

	for (int i = list->count; i > last; i--) {
		list->start[i] = list->start[i - 1];
	}

	list->start[last] = new_start;
	list->count++;

    return 0;
}

void clear_segment_start_list(struct SegmentStartList *list) {
    if (list->start) {
        free(list->start);
    }

    initialize_segment_start_list(list);
}
