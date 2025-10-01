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

const char *MATH_INSTRUCTION[] = {
    "test", NULL, "not", "neg", "mul", "imul", "div", "idiv"
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

const char *LOOP_INSTRUCTIONS[] = {
	"loopnzw", "loopzw", "loopw", "jcxz"
};

const char *SHIFT_INSTRUCTIONS[] = {
    "rol", "ror", "rcl", "rcr", "shl", "shr", NULL, "sar"
};

const char *FF_INSTRUCTIONS[] = {
    "inc", "dec", "call", "call", "jmp", "jmp", "push" /*, NULL */
};

#define DUMP_GLOBAL_VARIABLE_UNDEFINED 0xFFFFFFFF

static void dump_address(
		struct Reader *reader,
		unsigned int reference_address,
        void (*print)(const char *),
        void (*print_variable_label)(void (*)(const char *), unsigned int),
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
			const int addr = read_next_word(reader);
            if (reference_address == DUMP_GLOBAL_VARIABLE_UNDEFINED) {
                print_literal_hex_word(print, addr);
            }
            else {
                print_variable_label(print, reference_address);
            }
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
		unsigned int reference_address,
        void (*print)(const char *),
        void (*print_variable_label)(void (*)(const char *), unsigned int),
		int value0,
		int value1,
		const char **registers,
		const char *segment,
		const char **addr_replacement_registers) {
	if (value0 & 0x02) {
		print(registers[(value1 >> 3) & 0x07]);
		print(",");
		dump_address(reader, reference_address, print, print_variable_label, value1, segment, addr_replacement_registers);
	}
	else {
		dump_address(reader, reference_address, print, print_variable_label, value1, segment, addr_replacement_registers);
		print(",");
		print(registers[(value1 >> 3) & 0x07]);
	}
}

static int dump_instruction(
        struct Reader *reader,
        const struct CodeBlock *block,
        unsigned int reference_address,
        unsigned int reference_variable_value,
        struct CodeBlock *reference_block_value,
        void (*print)(const char *),
        void (*print_error)(const char *),
        void (*print_code_label)(void (*)(const char *), int, int),
        void (*print_variable_label)(void (*)(const char *), unsigned int)) {
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
                dump_address_register_combination(reader, reference_address, print, print_variable_label, value0, value1, registers, segment, registers);
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
            const int target_ip = block->ip + reader->buffer_index + ((value1 >= 0x80)? value1 - 256 : value1);
            print(JUMP_INSTRUCTIONS[value0 & 0x0F]);
            print(" ");
            print_code_label(print, target_ip, block->relative_cs);
            print("\n");
            return 0;
        }
        else if ((value0 & 0xFE) == 0x80) {
            const int value1 = read_next_byte(reader);
            print(INSTRUCTION[(value1 >> 3) & 0x07]);
            if ((value1 & 0xC0) != 0xC0) {
                if (value0 & 1) {
                    print(" word ");
                }
                else {
                    print(" byte ");
                }
            }
            else {
                print(" ");
            }

            const char **registers = (value0 & 1)? WORD_REGISTERS : BYTE_REGISTERS;
            dump_address(reader, reference_address, print, print_variable_label, value1, segment, registers);
            print(",");
            if (value0 & 1) {
                print_literal_hex_word(print, read_next_word(reader));
            }
            else {
                print_literal_hex_byte(print, read_next_byte(reader));
            }
            print("\n");
            return 0;
        }
        else if (value0 == 0x83) {
            const int value1 = read_next_byte(reader);
            print(INSTRUCTION[(value1 >> 3) & 0x07]);
            if ((value1 & 0xC0) != 0xC0) {
                if (value0 & 1) {
                    print(" word ");
                }
                else {
                    print(" byte ");
                }
            }
            else {
                print(" ");
            }

            dump_address(reader, reference_address, print, print_variable_label, value1, segment, WORD_REGISTERS);
            print(",");
            print_differential_hex_byte(print, read_next_byte(reader));
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
            dump_address_register_combination(reader, reference_address, print, print_variable_label, value0, value1, registers, segment, registers);
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
                dump_address_register_combination(reader, reference_address, print, print_variable_label, value0, value1, SEGMENT_REGISTERS, segment, WORD_REGISTERS);
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

            const int addr_value = read_next_word(reader);
            if ((value0 & 0xFE) == 0xA0) {
                print(registers[0]);
                print(",[");
                if (segment) {
                    print(segment);
                    print(":");
                }

                if (reference_address == DUMP_GLOBAL_VARIABLE_UNDEFINED) {
                    print_literal_hex_word(print, addr_value);
                }
                else {
                    print_variable_label(print, reference_address);
                }

                print("]");
            }
            else {
                print("[");
                if (segment) {
                    print(segment);
                    print(":");
                }

                if (reference_address == DUMP_GLOBAL_VARIABLE_UNDEFINED) {
                    print_literal_hex_word(print, addr_value);
                }
                else {
                    print_variable_label(print, reference_address);
                }

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
        else if (value0 == 0xA8) {
            print("test al,");
            print_literal_hex_byte(print, read_next_byte(reader));
            print("\n");
            return 0;
        }
        else if (value0 == 0xA9) {
            print("test ax,");
            print_literal_hex_word(print, read_next_word(reader));
            print("\n");
            return 0;
        }
        else if (value0 == 0xAA) {
            print("stosb\n");
            return 0;
        }
        else if (value0 == 0xAB) {
            print("stosw\n");
            return 0;
        }
        else if (value0 == 0xAC) {
            print("lodsb\n");
            return 0;
        }
        else if (value0 == 0xAD) {
            print("lodsw\n");
            return 0;
        }
        else if (value0 == 0xAE) {
            print("scasb\n");
            return 0;
        }
        else if (value0 == 0xAF) {
            print("scasw\n");
            return 0;
        }
        else if ((value0 & 0xF0) == 0xB0) {
            print("mov ");
            if (value0 & 0x08) {
                print(WORD_REGISTERS[value0 & 0x07]);
                print(",");
                const int offset_value = read_next_word(reader);
                if (reference_variable_value != DUMP_GLOBAL_VARIABLE_UNDEFINED) {
                    print_variable_label(print, reference_variable_value);
                }
                else if (reference_block_value) {
                    print_code_label(print, reference_block_value->ip, reference_block_value->relative_cs);
                }
                else {
                    print_literal_hex_word(print, offset_value);
                }
            }
            else {
                print(BYTE_REGISTERS[value0 & 0x07]);
                print(",");
                print_literal_hex_byte(print, read_next_byte(reader));
            }
            print("\n");
            return 0;
        }
        else if (value0 == 0xC2) {
            print("ret ");
            print_literal_hex_word(print, read_next_word(reader));
            print("\n");
            return 0;
        }
        else if (value0 == 0xC3) {
            print("ret\n");
            return 0;
        }
        else if ((value0 & 0xFE) == 0xC4) {
            const int value1 = read_next_byte(reader);
            if ((value1 & 0xC0) == 0xC0) {
                print("db ");
                print_literal_hex_byte(print, value0);
                print(" ");
                print_literal_hex_byte(print, value1);
                print(" ; Unknown instruction\n");
                return 1;
            }
            else {
                if (value0 & 1) {
                    print("lds ");
                }
                else {
                    print("les ");
                }

                print(WORD_REGISTERS[(value1 >> 3) & 0x07]);
		        print(",");
		        dump_address(reader, reference_address, print, print_variable_label, value1, segment, NULL);
                print("\n");
                return 0;
            }
        }
        else if (value0 == 0xC6) {
            const int value1 = read_next_byte(reader);
            if (value1 & 0x38) {
                print("db ");
                print_literal_hex_byte(print, value0);
                print(" ");
                print_literal_hex_byte(print, value1);
                print(" ; Unknown instruction\n");
                return 1;
            }
            else {
                print("mov ");
                if ((value1 & 0xC0) != 0xC0) {
                    print("byte ");
                }
                dump_address(reader, reference_address, print, print_variable_label, value1, segment, BYTE_REGISTERS);
                print(",");
                print_literal_hex_byte(print, read_next_byte(reader));
                print("\n");
                return 0;
            }
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
        else if ((value0 & 0xFC) == 0xD0) {
            const int value1 = read_next_byte(reader);
            if ((value1 & 0x38) == 0x30) {
                print("db ");
                print_literal_hex_byte(print, value0);
                print(" ");
                print_literal_hex_byte(print, value1);
                print(" ; Unknown instruction\n");
                return 1;
            }
            else {
                print(SHIFT_INSTRUCTIONS[(value1 >> 3) & 0x07]);
                if ((value0 & 0xC0) == 0xC0) {
                    print(" ");
                }
                else if (value1 & 1) {
                    print(" word ");
                }
                else {
                    print(" byte ");
                }

                const char **registers = (value0 & 1)? WORD_REGISTERS : BYTE_REGISTERS;
                dump_address(reader, reference_address, print, print_variable_label, value1, segment, registers);

                if (value0 & 2) {
                    print(",cl\n");
                }
                else {
                    print(",1\n");
                }
                return 0;
            }
        }
        else if ((value0 & 0xFC) == 0xE0) {
            const int value1 = read_next_byte(reader);
            const int target_ip = block->ip + reader->buffer_index + ((value1 >= 0x80)? value1 - 256 : value1);
            print(LOOP_INSTRUCTIONS[value0 & 0x0F]);
            print(" ");
            print_code_label(print, target_ip, block->relative_cs);
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
            
            print_code_label(print, block->ip + reader->buffer_index + diff, block->relative_cs);
            print("\n");
            return 0;
        }
        else if (value0 == 0xEB) {
            const int value1 = read_next_byte(reader);
            const int target_ip = block->ip + reader->buffer_index + ((value1 >= 0x80)? value1 - 256 : value1);
            print("jmp ");
            print_code_label(print, target_ip, block->relative_cs);
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
        else if ((value0 & 0xFE) == 0xF6) {
            const int value1 = read_next_byte(reader);
            if ((value1 & 0x38) == 0x08) {
                print("db ");
                print_literal_hex_byte(print, value0);
                print(" ");
                print_literal_hex_byte(print, value1);
                print(" ; Unknown instruction\n");
                return 1;
            }
            else {
                print(MATH_INSTRUCTION[(value1 >> 3) & 0x07]);
                if ((value1 & 0xC0) != 0xC0) {
                    if (value0 & 1) {
                        print(" word ");
                    }
                    else {
                        print(" byte ");
                    }
                }
                else {
                    print(" ");
                }

                const char **registers = (value0 & 1)? WORD_REGISTERS : BYTE_REGISTERS;
                dump_address(reader, reference_address, print, print_variable_label, value1, segment, registers);
                if ((value0 & 0x38) == 0) {
                    print(",");
                    if (value0 & 1) {
                        print_literal_hex_word(print, read_next_word(reader));
                    }
                    else {
                        print_literal_hex_byte(print, read_next_byte(reader));
                    }
                }
                print("\n");
                return 0;
            }
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
        else if (value0 == 0xFF) {
            const int value1 = read_next_byte(reader);
            if ((value1 & 0x38) == 0x38 || (value1 & 0xF8) == 0xD8 || (value1 & 0xF8) == 0xE8) {
                print_error("Unknown opcode ");
                print_literal_hex_byte(print_error, value0);
                print_error(" ");
                print_literal_hex_byte(print_error, value1);
                print_error("\n");
                return 1;
            }
            else {
                print(FF_INSTRUCTIONS[(value1 >> 3) & 0x07]);
                if (value1 < 0xC0 && ((value1 & 0x08) == 0x00 || (value1 & 0x38) == 0x08)) {
                    print(" word ");
                }
                else if (value1 < 0xC0 && ((value1 & 0x38) == 0x18 || (value1 & 0x38) == 0x28)) {
                    print(" far16 ");
                }
                else {
                    print(" ");
                }
                dump_address(reader, reference_address, print, print_variable_label, value1, segment, WORD_REGISTERS);
                print("\n");
                return 0;
            }
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
        struct Reference **references,
        unsigned int global_variable_reference_count,
        void (*print)(const char *),
        void (*print_error)(const char *),
        void (*print_code_label)(void (*)(const char *), int, int),
        void (*print_variable_label)(void (*)(const char *), unsigned int)) {
    struct Reader reader;
    reader.buffer = block->start;
    reader.buffer_index = 0;
    reader.buffer_size = block->end - block->start;

    print("\n");
    print_code_label(print, block->ip, block->relative_cs);
    print(":\n");

    int error_found = 0;
    do {
        if (error_found) {
            print("db ");
            print_literal_hex_byte(print, read_next_byte(&reader));
            print("\n");
        }
        else {
            while (global_variable_reference_count > 0 && references[0]->instruction < reader.buffer + reader.buffer_index) {
                references++;
                global_variable_reference_count--;
            }

            unsigned int global_variable_reference_address = DUMP_GLOBAL_VARIABLE_UNDEFINED;
            unsigned int global_variable_reference_value = DUMP_GLOBAL_VARIABLE_UNDEFINED;
            struct CodeBlock *reference_block_value = NULL;
            if (global_variable_reference_count > 0 && references[0]->instruction == reader.buffer + reader.buffer_index) {
                struct Reference *reference = references[0];
                if (reference->address) {
                    global_variable_reference_address = reference->address->relative_address;
                }

                if (reference->variable_value) {
                    global_variable_reference_value = reference->variable_value->relative_address;
                }

                reference_block_value = reference->block_value;
                references++;
                global_variable_reference_count--;
            }

            error_found = dump_instruction(&reader, block, global_variable_reference_address, global_variable_reference_value, reference_block_value, print, print_error, print_code_label, print_variable_label);
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
        void (*print_error)(const char *),
        void (*print_variable_label)(void (*)(const char *), unsigned int)) {
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
        struct Reference **global_variable_references,
        unsigned int global_variable_reference_count,
        void (*print)(const char *),
        void (*print_error)(const char *),
        void (*print_code_label)(void (*)(const char *), int, int),
        void (*print_variable_label)(void (*)(const char *), unsigned int)) {
    struct Reader reader;
    int error_code;

    int next_code_block_index = 0;
    int next_global_variable_index = 0;

    while (next_code_block_index < code_block_count || next_global_variable_index < global_variable_count) {
        if (next_code_block_index < code_block_count && (next_global_variable_index == global_variable_count || sorted_blocks[next_code_block_index]->start <= sorted_variables[next_global_variable_index]->start)) {
            struct CodeBlock *block = sorted_blocks[next_code_block_index++];
            while(global_variable_reference_count > 0 && global_variable_references[0]->instruction < block->start) {
                global_variable_references++;
                global_variable_reference_count--;
            }

            if ((error_code = dump_block(block, global_variable_references, global_variable_reference_count, print, print_error, print_code_label, print_variable_label))) {
                return error_code;
            }
        }
        else {
            if ((error_code = dump_variable(sorted_variables[next_global_variable_index++], print, print_error, print_variable_label))) {
                return error_code;
            }
        }
    }

    return 0;
}
