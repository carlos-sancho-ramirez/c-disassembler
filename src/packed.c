#include "packed.h"
#include <stdlib.h>

packed_data_t *allocate_bitset(unsigned int bit_count) {
	const unsigned int bits_per_word = sizeof(packed_data_t) * 8;
	const unsigned int required_words = (bit_count + bits_per_word - 1) / bits_per_word;
	return calloc(required_words, sizeof(packed_data_t));
}

int are_bitsets_equal(const packed_data_t *a, const packed_data_t *b, unsigned int bit_count) {
	const unsigned int bits_per_word = sizeof(packed_data_t) * 8;
	const unsigned int allocated_words = (bit_count + bits_per_word - 1) / bits_per_word;
	int i;

	for (i = 0; i < allocated_words; i++) {
		if (a[i] != b[i]) {
			return 0;
		}
	}

	return 1;
}

unsigned int count_set_bits_in_bitset(const packed_data_t *bitset, unsigned int bit_count) {
	const unsigned int bits_per_word = sizeof(packed_data_t) * 8;
	const unsigned int allocated_words = (bit_count + bits_per_word - 1) / bits_per_word;
	int count = 0;
	int word_index;

	for (word_index = 0; word_index < allocated_words; word_index++) {
		packed_data_t word = bitset[word_index];
		const int limit = (word_index + 1 < allocated_words || bit_count % bits_per_word == 0)? bits_per_word : bit_count % bits_per_word;
		int bit_index;
		for (bit_index = 0; bit_index < bits_per_word; bit_index++) {
			if (word & 1) {
				count++;
			}

			word >>= 1;
		}
	}

	return count;
}

int get_bitset_value(const packed_data_t *bitset, unsigned int index) {
	const unsigned int bits_per_word = sizeof(packed_data_t) * 8;
	const unsigned int word_index = index / bits_per_word;
	const packed_data_t mask = 1 << index % bits_per_word;
	return bitset[word_index] & mask;
}

void set_bitset_value(packed_data_t *bitset, unsigned int index, int value) {
	const unsigned int bits_per_word = sizeof(packed_data_t) * 8;
	const unsigned int word_index = index / bits_per_word;
	const packed_data_t mask = 1 << index % bits_per_word;

	if (value) {
		bitset[word_index] |= mask;
	}
	else {
		bitset[word_index] &= ~mask;
	}
}
