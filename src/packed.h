#ifndef _PACKED_H_
#define _PACKED_H_

typedef unsigned int packed_data_t;

/**
 * Initialise an array of packed_data_t to be used as a bitset.
 *
 * This will allocate enough memory to ensure that there is at least the given
 * number of bits. This method will also provide the allocated memory with all
 * bits set to 0.
 *
 * Callers of this methods are responsible to call method free with the
 * returned pointer.
 */
packed_data_t *allocate_bitset(unsigned int bit_count);

/**
 * Check if both bitsets have all their bits in the same state.
 */
int are_bitsets_equal(const packed_data_t *a, const packed_data_t *b, unsigned int bit_count);

/**
 * Return the number of bits set in this bitset.
 */
unsigned int count_set_bits_in_bitset(const packed_data_t *bitset, unsigned int bit_count);

/**
 * Checkes the bit at the given index within the given bitset and returns 0 if
 * the bit is clear, or any other value if set.
 */
int get_bitset_value(const packed_data_t *bitset, unsigned int index);

/**
 * Modifies the bit at the given index within the given bitset.
 * It will result in 1 if the given value is different from zero, or 0 otherwise.
 */
void set_bitset_value(packed_data_t *bitset, unsigned int index, int value);

#ifdef DEBUG
void print_bitset(const packed_data_t *bitset, unsigned int bit_count);
#endif /* DEBUG */

#endif /* _PACKED_H_ */
