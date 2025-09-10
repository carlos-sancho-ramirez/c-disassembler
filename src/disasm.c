#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE (1 << 14)

struct Reader {
	const char *buffer;
	int buffer_size;
	int buffer_index;
};

static int read_next_byte(struct Reader *reader) {
	return reader->buffer[(reader->buffer_index)++] & 0xFF;
}

static int read_next_word(struct Reader *reader) {
	return read_next_byte(reader) + (read_next_byte(reader) << 8);
}

const char HEX_CHAR[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static void print_literal_hex_byte(void (*print)(const char *), int value) {
	char number[] = "0x00";
	number[3] = HEX_CHAR[value & 0x0F];
	number[2] = HEX_CHAR[(value >> 4) & 0x0F];
	print(number);
}

static void print_literal_hex_word(void (*print)(const char *), int value) {
	char number[] = "0x0000";
	number[5] = HEX_CHAR[value & 0x000F];
	number[4] = HEX_CHAR[(value >> 4) & 0x000F];
	number[3] = HEX_CHAR[(value >> 8) & 0x000F];
	number[2] = HEX_CHAR[(value >> 12) & 0x000F];
	print(number);
}

static int dump_instruction(struct Reader *reader, void (*print)(const char *), void (*print_error)(const char *)) {
    const char *segment = NULL;
	while (1) {
		const int value0 = read_next_byte(reader);
		if (value0 == 0x26) {
			segment = "es:";
		}
		else if (value0 == 0x31) {
			const int value1 = read_next_byte(reader);
			if (value1 == 0xC0) {
				print("xor ax,ax\n");
				return 0;
			}
			else {
				print_error("Unknown opcode ");
				print_literal_hex_byte(print_error, value0);
				print_error(" ");
				print_literal_hex_byte(print_error, value1);
				print_error("\n");
				return 1;
			}
		}
		else if (value0 == 0x48) {
			print("dec ax\n");
			return 0;
		}
		else if (value0 == 0x8E) {
			const int value1 = read_next_byte(reader);
			if (value1 == 0xC0) {
				print("mov es,ax\n");
				return 0;
			}
			else {
				print_error("Unknown opcode ");
				print_literal_hex_byte(print_error, value0);
				print_error(" ");
				print_literal_hex_byte(print_error, value1);
				print_error("\n");
				return 1;
			}
		}
		else if (value0 == 0xA1) {
			print("mov ax,[");
			if (segment) {
				print(segment);
			}
			print_literal_hex_word(print, read_next_word(reader));
			print("]\n");
		}
		else if (value0 == 0xB4) {
			print("mov ah,");
			print_literal_hex_byte(print, read_next_byte(reader));
			print("\n");
			return 0;
		}
		else if (value0 == 0xBA) {
			print("mov dx,");
			print_literal_hex_word(print, read_next_word(reader));
			print("\n");
			return 0;
		}
		else if (value0 == 0xCD) {
			print("int ");
			print_literal_hex_byte(print, read_next_byte(reader));
			print("\n");
			return 0;
		}
		else {
			print_error("Unknown opcode ");
			print_literal_hex_byte(print_error, value0);
			print_error("\n");
			return 1;
		}
	}
}

static int dump(struct Reader *reader, void (*print)(const char *), void (*print_error)(const char *)) {
	int result;
	while (reader->buffer_index < reader->buffer_size) {
		if (result = dump_instruction(reader, print, print_error)) {
			return result;
		}
	}
}

static void print_help(const char *executedFile) {
	printf("Syntax: %s <options>\nPossible options:\n  -h or --help      Show this help.\n  -i <filename>     Uses this file as input.\n", executedFile);
}

static void dump_print(const char *str) {
	printf(str);
}

static void print_error(const char *str) {
	fprintf(stderr, str);
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

	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Unable to open file\n");
		return 1;
	}

	struct Reader reader;
	reader.buffer = malloc(BUFFER_SIZE);
	if (!reader.buffer) {
		fprintf(stderr, "Unable to allocate memory\n");
		fclose(file);
		return 1;
	}

	reader.buffer_size = fread(reader.buffer, 1, BUFFER_SIZE, file);
	if (!reader.buffer_size) {
		fprintf(stderr, "Unable to read file\n");
		free(reader.buffer);
		fclose(file);
		return 1;
	}
	reader.buffer_index = 0;

	int result = dump(&reader, dump_print, print_error);
	free(reader.buffer);
	fclose(file);
	return result;
}
