#include "renames.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printd.h"

#define PARSER_STATE_FINDING_KEY_START 0
#define PARSER_STATE_FINDING_KEY_END 1
#define PARSER_STATE_FINDING_VALUE_START 2
#define PARSER_STATE_FINDING_VALUE_END 3
#define PARSER_STATE_FINDING_LINE_END 4

struct EntryLimits {
	unsigned int key_start;
	unsigned int key_end;
	unsigned int value_start;
	unsigned int value_end;
};

#define SYMBOL_MAX_LENGTH 127
#define BUFFER_SIZE 1024
#define ENTRIES_PER_PAGE 32
#define BYTES_PER_BUFFER_PAGE 1024

struct MutableRenameMap {
	struct KeyValuePair *entries;
	char *buffer;
	unsigned int entry_count;
	unsigned int buffer_in_use;
};

static int insert_entry(struct MutableRenameMap *map, const struct EntryLimits *entry_limits, const char *file_buffer, const char *filename, unsigned int line) {
	unsigned int key_length = entry_limits->key_end - entry_limits->key_start;
	unsigned int value_length = entry_limits->value_end - entry_limits->value_start;
	unsigned int allocated_buffer_pages = (map->buffer_in_use + BYTES_PER_BUFFER_PAGE - 1) / BYTES_PER_BUFFER_PAGE;
	unsigned int new_allocated_buffer_pages = (map->buffer_in_use + key_length + value_length + BYTES_PER_BUFFER_PAGE + 1) / BYTES_PER_BUFFER_PAGE;

	char *key;
	char *value;

	unsigned int first;
	unsigned int last;
	unsigned int index;

	if (key_length > SYMBOL_MAX_LENGTH) {
		fprintf(stderr, "Error at %s:%d. Key is too long.\n", filename, line);
		return 1;
	}

	if (value_length > SYMBOL_MAX_LENGTH) {
		fprintf(stderr, "Error at %s:%d. Value is too long.\n", filename, line);
		return 1;
	}

	if (map->entry_count % ENTRIES_PER_PAGE == 0) {
		map->entries = realloc(map->entries, sizeof(struct KeyValuePair) * (map->entry_count + ENTRIES_PER_PAGE));
		if (!map->entries) {
			fprintf(stderr, "Unable to allocate %lu contiguous bytes in memory.\n", sizeof(struct KeyValuePair) * (map->entry_count + ENTRIES_PER_PAGE));
			return 1;
		}
	}

	if (new_allocated_buffer_pages > allocated_buffer_pages) {
		map->buffer = realloc(map->buffer, new_allocated_buffer_pages * BYTES_PER_BUFFER_PAGE);
		if (!map->buffer) {
			fprintf(stderr, "Unable to allocate %d contiguous bytes in memory.\n", new_allocated_buffer_pages * BYTES_PER_BUFFER_PAGE);
			return 1;
		}
	}

	key = map->buffer + map->buffer_in_use;
	map->buffer_in_use += key_length + 1;

	value = map->buffer + map->buffer_in_use;
	map->buffer_in_use += value_length + 1;

	strncpy(key, file_buffer + entry_limits->key_start, key_length);
	key[key_length] = '\0';

	strncpy(value, file_buffer + entry_limits->value_start, value_length);
	value[value_length] = '\0';

	first = 0;
	last = map->entry_count;

	while (first < last) {
		const char *this_key;
		int comp;

		index = (first + last) / 2;
		this_key = map->entries[index].key;
		comp = strcmp(key, this_key);

		if (comp < 0) {
			last = index;
		}
		else if (comp > 0) {
			first = index + 1;
		}
		else {
			fprintf(stderr, "Key '%s' is duplicated.\n", key);
			return 1;
		}
	}

	for (index = map->entry_count; index > first; index--) {
		map->entries[index].key = map->entries[index - 1].key;
		map->entries[index].value = map->entries[index - 1].value;
	}

	map->entries[first].key = key;
	map->entries[first].value = value;
	map->entry_count++;
	return 0;
}

int read_renames_file(struct RenameMap *map, const char *filename) {
	FILE *file;
	char file_buffer[BUFFER_SIZE];
	unsigned int remaining_buffer;
	unsigned int buffer_index;
	unsigned int parser_state;
	struct EntryLimits entry_limits;
	unsigned int line;
	struct MutableRenameMap mutable_map;
	unsigned int index;

	mutable_map.buffer = NULL;
	mutable_map.buffer_in_use = 0;
	mutable_map.entries = NULL;
	mutable_map.entry_count = 0;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Unable to open file %s\n", filename);
		return 1;
	}

	remaining_buffer = fread(file_buffer, 1, BUFFER_SIZE, file);
	if (remaining_buffer == BUFFER_SIZE) {
		fprintf(stderr, "Buffer size limit reached on reading rename map from file %s.\n", filename);
		return 1;
	}

	parser_state = PARSER_STATE_FINDING_KEY_START;
	line = 1;

	for (buffer_index = 0; buffer_index < remaining_buffer; buffer_index++) {
		const char ch = file_buffer[buffer_index];
		if (parser_state == PARSER_STATE_FINDING_KEY_START) {
			if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z') {
				entry_limits.key_start = buffer_index;
				parser_state = PARSER_STATE_FINDING_KEY_END;
			}
		}
		else if (parser_state == PARSER_STATE_FINDING_KEY_END) {
			if (ch == '\n') {
				fprintf(stderr, "Error at %s:%d. No value present.\n", filename, line);
			}
			else if (!(ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == '_')) {
				entry_limits.key_end = buffer_index;
				parser_state = PARSER_STATE_FINDING_VALUE_START;
			}
		}
		else if (parser_state == PARSER_STATE_FINDING_VALUE_START) {
			if (ch == '\n') {
				fprintf(stderr, "Error at %s:%d. No value present.\n", filename, line);
			}
			else if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z') {
				entry_limits.value_start = buffer_index;
				parser_state = PARSER_STATE_FINDING_VALUE_END;
			}
		}
		else if (parser_state == PARSER_STATE_FINDING_VALUE_END) {
			if (ch == '\n') {
				entry_limits.value_end = buffer_index;
				if (insert_entry(&mutable_map, &entry_limits, file_buffer, filename, line)) {
					return 1;
				}
				parser_state = PARSER_STATE_FINDING_KEY_START;
			}
			else if (!(ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == '_')) {
				entry_limits.value_end = buffer_index;
				if (insert_entry(&mutable_map, &entry_limits, file_buffer, filename, line)) {
					return 1;
				}
				parser_state = PARSER_STATE_FINDING_LINE_END;
			}
		}
		else { /* parser_state == PARSER_STATE_FINDING_LINE_END */
			if (ch == '\n') {
				parser_state = PARSER_STATE_FINDING_KEY_START;
			}
		}

		if (ch == '\n') {
			line++;
		}
	}
	fclose(file);

	map->entries = malloc(mutable_map.entry_count * sizeof(struct KeyValuePairConst) + mutable_map.buffer_in_use);
	if (!map->entries) {
		fprintf(stderr, "Unable to allocate %lu bytes.\n", mutable_map.entry_count * sizeof(struct KeyValuePairConst) + mutable_map.buffer_in_use);
	}

	map->entry_count = mutable_map.entry_count;

	memcpy(map->entries + mutable_map.entry_count, mutable_map.buffer, mutable_map.buffer_in_use);

	for (index = 0; index < mutable_map.entry_count; index++) {
		map->entries[index].key = (char *) (map->entries + mutable_map.entry_count) + (mutable_map.entries[index].key - mutable_map.buffer);
		map->entries[index].value = (char *) (map->entries + mutable_map.entry_count) + (mutable_map.entries[index].value - mutable_map.buffer);
	}

	free(mutable_map.entries);
	free(mutable_map.buffer);
	return 0;
}

int index_of_key_in_rename_map(const struct RenameMap *map, const char *key) {
	unsigned int first = 0;
	unsigned int last = map->entry_count;

	while (first < last) {
		const char *this_key;
		int comp;

		unsigned int index = (first + last) / 2;
		this_key = map->entries[index].key;
		comp = strcmp(key, this_key);

		if (comp < 0) {
			last = index;
		}
		else if (comp > 0) {
			first = index + 1;
		}
		else {
			return index;
		}
	}

	return -1;
}

#ifdef DEBUG
void print_rename_map(const struct RenameMap *map) {
	unsigned int index;
	fprintf(stderr, "Rename Map:\n");
	for (index = 0; index < map->entry_count; index++) {
		fprintf(stderr, " %s -> %s\n", map->entries[index].key, map->entries[index].value);
	}
}

#endif /* DEBUG */
