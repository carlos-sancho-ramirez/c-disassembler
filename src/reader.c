#include "reader.h"
#include "printd.h"

#ifdef DEBUG
int reader_debug_print_enabled = 0;
#endif /* DEBUG */

int read_next_byte(struct Reader *reader) {
	const int result = reader->buffer[(reader->buffer_index)++] & 0xFF;

#ifdef DEBUG
	if (reader_debug_print_enabled) {
		DEBUG_PRINT1(" %02X", result & 0xFF);
	}
#endif /* DEBUG */

	return result;
}

int read_next_word(struct Reader *reader) {
	return read_next_byte(reader) + (read_next_byte(reader) << 8);
}
