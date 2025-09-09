#include <stdio.h>
#include <string.h>

static void print_help(const char *executedFile) {
	printf("Syntax: %s <options>\nPossible options:\n  -h or --help      Show this help.\n  -i <filename>     Uses this file as input.\n", executedFile);
}

int main(int argc, const char *argv[]) {
	printf("Disassmebler\n");

	const char *filename = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_help(argv[0]);
			return 0;
		}
		else if (!strcmp(argv[i], "-i")) {
			if (++i < argc) {
				filename = argv[i];
			}
			else {
				fprintf(stderr, "Missing file name after -i argument\n");
				print_help(argv[0]);
				return 1;
			}
		}
		else {
			fprintf(stderr, "Unexpected argument %s\n", argv[i]);
			print_help(argv[0]);
			return 1;
		}
	}

	if (!filename) {
		fprintf(stderr, "Argument -i is required\n");
		print_help(argv[0]);
		return 1;
	}

	printf("Now let's read file %s\n", filename);
	return 0;
}

