#include "relocations.h"

int is_relocation_present_in_sorted_relocations(const char **sorted_relocations, unsigned int relocation_count, const char *relocation_query) {
	int first = 0;
	int last = relocation_count;
	while (last > first) {
		int index = (first + last) / 2;
		const char *this_relocation = sorted_relocations[index];
		if (this_relocation < relocation_query) {
			first = index + 1;
		}
		else if (this_relocation > relocation_query) {
			last = index;
		}
		else {
			return 1;
		}
	}

	return 0;
}
