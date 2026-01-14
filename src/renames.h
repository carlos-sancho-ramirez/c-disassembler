#ifndef _RENAMES_H_
#define _RENAMES_H_

struct KeyValuePair {
	char *key;
	char *value;
};

struct KeyValuePairConst {
	const char *key;
	const char *value;
};

struct RenameMap {
	struct KeyValuePairConst *entries;
	unsigned int entry_count;
};

int read_renames_file(struct RenameMap *map, const char *filename);
void free_rename_map(struct RenameMap *map);

int index_of_key_in_rename_map(const struct RenameMap *map, const char *key);
#ifdef DEBUG
void print_rename_map(const struct RenameMap *map);
#endif /* DEBUG */

#endif /* _RENAMES_H_ */
