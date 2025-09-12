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

static void print_differential_hex_byte(void (*print)(const char *), int value) {
	char number[] = "+0x00";
	if (value & 0x80) {
		number[0] = '-';
		value = (value ^ 0xFF) + 1;
	}

	number[4] = HEX_CHAR[value & 0x0F];
	number[3] = HEX_CHAR[(value >> 4) & 0x0F];
	print(number);
}

static void print_differential_hex_word(void (*print)(const char *), int value) {
	char number[] = "+0x0000";
	if (value & 0x8000) {
		number[0] = '-';
		value = (value ^ 0xFFFF) + 1;
	}

	number[6] = HEX_CHAR[value & 0x000F];
	number[5] = HEX_CHAR[(value >> 4) & 0x000F];
	number[4] = HEX_CHAR[(value >> 8) & 0x000F];
	number[3] = HEX_CHAR[(value >> 12) & 0x000F];
	print(number);
}

const char *BYTE_REGISTERS[] = {
	"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

const char *WORD_REGISTERS[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

const char *INSTRUCTION[] = {
	"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"
};

const char *ADDRESS_REGISTERS[] = {
	"bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"
};

const char *SEGMENT_REGISTERS[] = {
	"es", "cs", "ss", "ds"
};

static void dump_address(
		struct Reader *reader,
		void (*print)(const char *),
		int value1,
		const char *segment,
		const char **registers) {
	if ((value1 & 0xC0) != 0xC0) {
		print("[");
		if (segment) {
			print(segment);
			print(":");
		}

		if ((value1 & 0xC7) == 0x06) {
			print(read_next_word(reader));
		}
		else {
			print(ADDRESS_REGISTERS[value1 & 0x07]);
			if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_byte(reader, read_next_byte(reader));
			}
			else if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_word(reader, read_next_byte(reader));
			}
		}
		print("]");
	}
	else {
		print(registers[value1 & 0x07]);
	}
}

static int dump_instruction(struct Reader *reader, void (*print)(const char *), void (*print_error)(const char *)) {
    const char *segment = NULL;
	while (1) {
		const int value0 = read_next_byte(reader);
		if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
			print(INSTRUCTION[value0 >> 3]);
			print(" ");
			if ((value0 & 0x04) == 0x00) {
				const char **registers;
				if (value0 & 0x01) {
					registers = WORD_REGISTERS;
				}
				else {
					registers = BYTE_REGISTERS;
				}

				const int value1 = read_next_byte(reader);
				if ((value0 & 0x06) == 0x00) {
					dump_address(reader, print, value1, segment, registers);
					print(",");
					print(registers[(value1 >> 3) & 0x07]);
				}
				else if ((value0 & 0x06) == 0x02) {
					print(registers[(value1 >> 3) & 0x07]);
					print(",");
					dump_address(reader, print, value1, segment, registers);
				}
				print("\n");
				return 0;
			}
			else if ((value0 & 0x07) == 0x04) {
				print(BYTE_REGISTERS[0]);
				print(",");
				print_literal_hex_byte(print, read_next_byte(reader));
				print("\n");
				return 0;
			}
			else if ((value0 & 0x07) == 0x05) {
				print(WORD_REGISTERS[0]);
				print(",");
				print_literal_hex_word(print, read_next_word(reader));
				print("\n");
				return 0;
			}
		}
		else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
			if (value0 & 0x01) {
				print("pop ");
			}
			else {
				print("push ");
			}
			print(SEGMENT_REGISTERS[(value0 >> 3) & 0x03]);
			print("\n");
			return 0;
		}
		else if ((value0 & 0xE7) == 0x26) {
			segment = SEGMENT_REGISTERS[(value0 >> 3) & 0x03];
		}
		else if ((value0 & 0xF0) == 0x40) {
			if (value0 & 0x08) {
				print("dec ");
			}
			else {
				print("inc ");
			}
			print(WORD_REGISTERS[value0 & 0x07]);
			print("\n");
			return 0;
		}
		else if (value0 == 0x8E) {
			const int value1 = read_next_byte(reader);
			if (value1 & 0x20) {
				print_error("Unknown opcode ");
				print_literal_hex_byte(print_error, value0);
				print_error(" ");
				print_literal_hex_byte(print_error, value1);
				print_error("\n");
				return 1;
			}
			else {
				print("mov ");
				print(SEGMENT_REGISTERS[(value1 >> 3) & 0x03]);
				print(",");
				dump_address(reader, print, value1, segment, WORD_REGISTERS);
				print("\n");
				return 0;
			}
		}
		else if ((value0 & 0xFC) == 0xA0) {
			print("mov ");
			const char **registers;
			if (value0 & 1) {
				registers = WORD_REGISTERS;
			}
			else {
				registers = BYTE_REGISTERS;
			}

			if ((value0 & 0xFE) == 0xA0) {
				print(registers[0]);
				print(",[");
				if (segment) {
					print(segment);
					print(":");
				}
				print_literal_hex_word(print, read_next_word(reader));
				print("]");
			}
			else if ((value0 & 0xFE) == 0xA2) {
				print("[");
				if (segment) {
					print(segment);
					print(":");
				}
				print_literal_hex_word(print, read_next_word(reader));
				print("],");
				print(registers[0]);
			}
			print("\n");
			return 0;
		}
		else if ((value0 & 0xF0) == 0xB0) {
			print("mov ");
			if (value0 & 0x08) {
				print(WORD_REGISTERS[value0 & 0x07]);
				print(",");
				print_literal_hex_word(print, read_next_word(reader));
			}
			else {
				print(BYTE_REGISTERS[value0 & 0x07]);
				print(",");
				print_literal_hex_byte(print, read_next_byte(reader));
			}
			print("\n");
			return 0;
		}
		else if (value0 == 0xCD) {
			print("int ");
			print_literal_hex_byte(print, read_next_byte(reader));
			print("\n");
			return 0;
		}
		else if (value0 == 0xF8) {
			print("clc\n");
			return 0;
		}
		else if (value0 == 0xF9) {
			print("stc\n");
			return 0;
		}
		else if (value0 == 0xFA) {
			print("cli\n");
			return 0;
		}
		else if (value0 == 0xFB) {
			print("sti\n");
			return 0;
		}
		else if (value0 == 0xFC) {
			print("cld\n");
			return 0;
		}
		else if (value0 == 0xFD) {
			print("std\n");
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
