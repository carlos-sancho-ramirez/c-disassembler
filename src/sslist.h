#ifndef _SEGMENT_START_LIST_H_
#define _SEGMENT_START_LIST_H_

struct SegmentStartList {
	const char **start;
	unsigned int count;
};

void initialize_segment_start_list(struct SegmentStartList *list);
int contains_segment_start(struct SegmentStartList *list, const char *start);
int insert_segment_start(struct SegmentStartList *list, const char *new_start);
void clear_segment_start_list(struct SegmentStartList *list);

#endif /* _SEGMENT_START_LIST_H_ */
