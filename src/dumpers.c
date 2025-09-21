#include "dumpers.h"
#include "print_utils.h"
#include "reader.h"

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

const char *JUMP_INSTRUCTIONS[] = {
	"jo", "jno", "jb", "jae", "jz", "jnz", "jbe", "ja",
	"js", "jns", "jpe", "jpo", "jl", "jge", "jle", "jg"
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
			print_literal_hex_word(print, read_next_word(reader));
		}
		else {
			print(ADDRESS_REGISTERS[value1 & 0x07]);
			if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_byte(print, read_next_byte(reader));
			}
			else if ((value1 & 0xC0) == 0x40) {
				print_differential_hex_word(print, read_next_word(reader));
			}
		}
		print("]");
	}
	else {
		print(registers[value1 & 0x07]);
	}
}

static void dump_address_register_combination(
		struct Reader *reader,
		void (*print)(const char *),
		int value0,
		int value1,
		const char **registers,
		const char *segment,
		const char **addr_replacement_registers) {
	if (value0 & 0x02) {
		print(registers[(value1 >> 3) & 0x07]);
		print(",");
		dump_address(reader, print, value1, segment, addr_replacement_registers);
	}
	else {
		dump_address(reader, print, value1, segment, addr_replacement_registers);
		print(",");
		print(registers[(value1 >> 3) & 0x07]);
	}
}

static int dump_instruction(
        struct Reader *reader,
        const struct CodeBlock *block,
        void (*print)(const char *),
        void (*print_error)(const char *)) {
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
                dump_address_register_combination(reader, print, value0, value1, registers, segment, registers);
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
            else {
                // Assuming (value0 & 0x07) == 0x05
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
        else if ((value0 & 0xF0) == 0x50) {
            if (value0 & 0x08) {
                print("pop ");
            }
            else {
                print("push ");
            }
            print(WORD_REGISTERS[value0 & 0x07]);
            print("\n");
            return 0;
        }
        else if ((value0 & 0xF0) == 0x70) {
            const int value1 = read_next_byte(reader);
            print(JUMP_INSTRUCTIONS[value0 & 0x0F]);
            print(" ");
            print_address_label(print, block->ip + reader->buffer_index, block->relative_cs);
            print("\n");
            return 0;
        }
        else if ((value0 & 0xFC) == 0x88) {
            const char **registers;
            if (value0 & 1) {
                registers = WORD_REGISTERS;
            }
            else {
                registers = BYTE_REGISTERS;
            }

            const int value1 = read_next_byte(reader);
            print("mov ");
            dump_address_register_combination(reader, print, value0, value1, registers, segment, registers);
            print("\n");
            return 0;
        }
        else if ((value0 & 0xFD) == 0x8C) {
            const int value1 = read_next_byte(reader);
            if (value1 & 0x20) {
                print("db ");
                print_literal_hex_byte(print, value0);
                print(" ");
                print_literal_hex_byte(print, value1);
                print(" ; Unknown instruction\n");
                return 1;
            }
            else {
                print("mov ");
                dump_address_register_combination(reader, print, value0, value1, SEGMENT_REGISTERS, segment, WORD_REGISTERS);
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
        else if (value0 == 0xA4) {
            print("movsb\n");
            return 0;
        }
        else if (value0 == 0xA5) {
            print("movsw\n");
            return 0;
        }
        else if (value0 == 0xA6) {
            print("cmpsb\n");
            return 0;
        }
        else if (value0 == 0xA7) {
            print("cmpsw\n");
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
        else if (value0 == 0xC3) {
            print("ret\n");
            return 0;
        }
        else if (value0 == 0xCB) {
            print("retf\n");
            return 0;
        }
        else if (value0 == 0xCD) {
            print("int ");
            print_literal_hex_byte(print, read_next_byte(reader));
            print("\n");
            return 0;
        }
        else if ((value0 & 0xFE) == 0xE8) {
            int diff = read_next_word(reader);
            if (block->ip + reader->buffer_index + diff >= 0x10000) {
                diff -= 0x10000;
            }

            if (value0 & 1) {
                print("jmp ");
            }
            else {
                print("call ");
            }
            
            print_address_label(print, block->ip + reader->buffer_index + diff, block->relative_cs);
            print("\n");
            return 0;
        }
        else if (value0 == 0xF2) {
            print("repne\n");
            return 0;
        }
        else if (value0 == 0xF3) {
            print("repe\n");
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
            print("db ");
            print_literal_hex_byte(print, value0);
            print(" ; Unknown instruction\n");
            return 1;
        }
    }
}

static int dump_block(
        const struct CodeBlock *block,
        void (*print)(const char *),
        void (*print_error)(const char *)) {
    struct Reader reader;
    reader.buffer = block->start;
    reader.buffer_index = 0;
    reader.buffer_size = block->end - block->start;

    print("\n");
    print_address_label(print, block->ip, block->relative_cs);
    print(":\n");

    int error_found = 0;
    do {
        if (error_found) {
            print("db ");
            print_literal_hex_byte(print, read_next_byte(&reader));
            print("\n");
        }
        else {
            error_found = dump_instruction(&reader, block, print, print_error);
        }
    }
    while (block->start + reader.buffer_index != block->end);

    return 0;
}

static int valid_char_for_string_literal(char ch) {
    // More can be added when required. Avoid adding quotes, as it may conflict
    return ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == ' ' || ch == '!' || ch == '$';
}

static int dump_variable(
        const struct GlobalVariable *variable,
        void (*print)(const char *),
        void (*print_error)(const char *)) {
    print("\n");
    print_variable_label(print, variable->relative_address);
    print(":\n");

    int should_display_string_literal = 0;
    if (variable->var_type == GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING) {
        should_display_string_literal = 1;
        for (const char *position = variable->start; position < variable->end; position++) {
            if (!valid_char_for_string_literal(*position)) {
                should_display_string_literal = 0;
                break;
            }
        }
    }

    if (should_display_string_literal) {
        print("db '");
        char str[] = "x";
        for (const char *position = variable->start; position < variable->end; position++) {
            str[0] = *position;
            print(str);
        }
        print("'\n");
    }
    else {
        for (const char *position = variable->start; position < variable->end; position++) {
            print("db ");
            print_literal_hex_byte(print, *position);
            print("\n");
        }
    }

    return 0;
}

int dump(
        struct CodeBlock **sorted_blocks,
        unsigned int code_block_count,
        struct GlobalVariable **sorted_variables,
        unsigned int global_variable_count,
        void (*print)(const char *),
        void (*print_error)(const char *)) {
    struct Reader reader;
    int error_code;

    int next_code_block_index = 0;
    int next_global_variable_index = 0;

    while (next_code_block_index < code_block_count || next_global_variable_index < global_variable_count) {
        if (next_code_block_index < code_block_count && (next_global_variable_index == global_variable_count || sorted_blocks[next_code_block_index]->start <= sorted_variables[next_global_variable_index]->start)) {
            if ((error_code = dump_block(sorted_blocks[next_code_block_index++], print, print_error))) {
                return error_code;
            }
        }
        else {
            if ((error_code = dump_variable(sorted_variables[next_global_variable_index++], print, print_error))) {
                return error_code;
            }
        }
    }

    return 0;
}
