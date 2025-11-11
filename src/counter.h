#ifndef _COUNTER_H_
#define _COUNTER_H_

#include "reader.h"

#define READ_ERROR_MAX_EXCEEDED 1
#define READ_ERROR_UNKNOWN_OPCODE 2

/**
 * Reads the instruction pointed by the given Reader, and adds to the reader
 * index the length of the instruction found.
 *
 * This method can be used to find the length in bytes of the instruction pointed.
 *
 * This method will return 0 if all goes fine, or a different value in case
 * either the end of the buffer is reached but the instruction is not yet
 * complete, or there is an unknown opcode.
 */
int read_for_instruction_length(struct Reader *reader);

#endif /* _COUNTER_H_ */
