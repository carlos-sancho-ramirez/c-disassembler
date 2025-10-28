#ifndef _READER_H_
#define _READER_H_

struct Reader {
	const char *buffer;
	unsigned int buffer_size;
	unsigned int buffer_index;
};

int read_next_byte(struct Reader *reader);
int read_next_word(struct Reader *reader);

#endif
