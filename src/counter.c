#include "counter.h"

static int move_reader_forward(struct Reader *reader, unsigned int byte_count) {
	reader->buffer_index += byte_count;
	return (reader->buffer_index <= reader->buffer_size)? 0 :
			READ_ERROR_MAX_EXCEEDED;
}

static int move_reader_forward_for_address_length(struct Reader *reader, int value1) {
	if (value1 < 0xC0) {
		if (value1 >= 0x80 || (value1 & 0xC7) == 0x06) {
			return move_reader_forward(reader, 2);
		}
		else if ((value1 & 0xC0) == 0x40) {
			return move_reader_forward(reader, 1);
		}
	}

	return 0;
}

int read_for_instruction_length(struct Reader *reader) {
	int value0;
	if (reader->buffer_index >= reader->buffer_size) {
		return READ_ERROR_MAX_EXCEEDED;
	}
	value0 = read_next_byte(reader);

	if (value0 >= 0 && value0 < 0x40 && (value0 & 0x06) != 0x06) {
		if ((value0 & 0x04) == 0x00) {
			int value1;
			if (reader->buffer_index >= reader->buffer_size) {
				return READ_ERROR_MAX_EXCEEDED;
			}

			value1 = read_next_byte(reader);
			return move_reader_forward_for_address_length(reader, value1);
		}
		else if ((value0 & 0x07) == 0x04) {
			return move_reader_forward(reader, 1);
		}
		else {
			/* (value0 & 0x07) == 0x05 */
			return move_reader_forward(reader, 2);
		}
	}
	else if ((value0 & 0xE6) == 0x06 && value0 != 0x0F) {
		return 0;
	}
	else if ((value0 & 0xE7) == 0x26) {
		return read_for_instruction_length(reader);
	}
	else if ((value0 & 0xF0) == 0x40) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0x50) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0x70) {
		return move_reader_forward(reader, 1);
	}
	else if ((value0 & 0xFE) == 0x80) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}
		value1 = read_next_byte(reader);
		if (move_reader_forward_for_address_length(reader, value1)) {
			return READ_ERROR_MAX_EXCEEDED;
		}

		return move_reader_forward(reader, (value0 & 1) + 1);
	}
	else if (value0 == 0x83) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}
		value1 = read_next_byte(reader);
		if (move_reader_forward_for_address_length(reader, value1)) {
			return READ_ERROR_MAX_EXCEEDED;
		}
		return move_reader_forward(reader, 1);
	}
	else if ((value0 & 0xFE) == 0x86 || (value0 & 0xFC) == 0x88) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}
		value1 = read_next_byte(reader);
		return move_reader_forward_for_address_length(reader, value1);
	}
	else if ((value0 & 0xFD) == 0x8C) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}
		value1 = read_next_byte(reader);
		return (value1 & 0x20)? READ_ERROR_UNKNOWN_OPCODE :
				move_reader_forward_for_address_length(reader, value1);
	}
	else if (value0 == 0x8D) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}
		value1 = read_next_byte(reader);
		return (value1 >= 0xC0)? READ_ERROR_UNKNOWN_OPCODE :
				move_reader_forward_for_address_length(reader, value1);
	}
	else if (value0 == 0x8F) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}
		value1 = read_next_byte(reader);
		return (value1 & 0x38 || value1 >= 0xC0)? READ_ERROR_UNKNOWN_OPCODE :
				move_reader_forward_for_address_length(reader, value1);
	}
	else if ((value0 & 0xF8) == 0x90) {
		return 0;
	}
	else if ((value0 & 0xFC) == 0xA0) {
		return move_reader_forward(reader, 2);
	}
	else if ((value0 & 0xFC) == 0xA4) {
		return 0;
	}
	else if (value0 == 0xA8) {
		return move_reader_forward(reader, 1);
	}
	else if (value0 == 0xA9) {
		return move_reader_forward(reader, 2);
	}
	else if ((value0 & 0xFE) == 0xAA || (value0 & 0xFC) == 0xAC) {
		return 0;
	}
	else if ((value0 & 0xF0) == 0xB0) {
		return move_reader_forward(reader, (value0 & 0x08)? 2 : 1);
	}
	else if (value0 == 0xC2) {
		return move_reader_forward(reader, 2);
	}
	else if (value0 == 0xC3) {
		return 0;
	}
	else if ((value0 & 0xFE) == 0xC4) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}

		value1 = read_next_byte(reader);
		return ((value1 & 0xC0) == 0xC0)? READ_ERROR_UNKNOWN_OPCODE :
				move_reader_forward_for_address_length(reader, value1);
	}
	else if ((value0 & 0xFE) == 0xC6) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}

		value1 = read_next_byte(reader);
		if (value1 & 0x38) {
			return READ_ERROR_UNKNOWN_OPCODE;
		}
		else {
			if (move_reader_forward_for_address_length(reader, value1)) {
				return READ_ERROR_MAX_EXCEEDED;
			}
			return move_reader_forward(reader, (value0 & 1) + 1);
		}
	}
	else if (value0 == 0xCB) {
		return 0;
	}
	else if (value0 == 0xCD) {
		return move_reader_forward(reader, 1);
	}
	else if ((value0 & 0xFC) == 0xD0) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}

		value1 = read_next_byte(reader);
		return ((value1 & 0x38) == 0x30)? READ_ERROR_UNKNOWN_OPCODE :
				move_reader_forward_for_address_length(reader, value1);
	}
	else if ((value0 & 0xFC) == 0xE0) {
		return move_reader_forward(reader, 1);
	}
	else if ((value0 & 0xFE) == 0xE8) {
		return move_reader_forward(reader, 2);
	}
	else if (value0 == 0xEA) {
		return move_reader_forward(reader, 4);
	}
	else if (value0 == 0xEB) {
		return move_reader_forward(reader, 1);
	}
	else if ((value0 & 0xFE) == 0xF2) {
		return 0;
	}
	else if ((value0 & 0xFE) == 0xF6) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}

		value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x08) {
			return READ_ERROR_UNKNOWN_OPCODE;
		}
		else {
			if (move_reader_forward_for_address_length(reader, value1)) {
				return READ_ERROR_MAX_EXCEEDED;
			}
			return (value0 & 0x38)? 0 : move_reader_forward(reader, (value0 & 1) + 1);
		}
	}
	else if ((value0 & 0xFC) == 0xF8 || (value0 & 0xFE) == 0xFC) {
		return 0;
	}
	else if (value0 == 0xFE) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}

		value1 = read_next_byte(reader);
		if (value1 & 0x30) {
			return READ_ERROR_UNKNOWN_OPCODE;
		}
		else {
			return move_reader_forward_for_address_length(reader, value1);
		}
	}
	else if (value0 == 0xFF) {
		int value1;
		if (reader->buffer_index >= reader->buffer_size) {
			return READ_ERROR_MAX_EXCEEDED;
		}

		value1 = read_next_byte(reader);
		if ((value1 & 0x38) == 0x38 || (value1 & 0xF8) == 0xD8 || (value1 & 0xF8) == 0xE8) {
			return READ_ERROR_UNKNOWN_OPCODE;
		}
		else {
			return move_reader_forward_for_address_length(reader, value1);
		}
	}
	else {
		return READ_ERROR_UNKNOWN_OPCODE;
	}
}
