#ifndef _STACK_H_
#define _STACK_H_

#include <stdint.h>

typedef unsigned int packed_data_t;

struct Stack {
	/**
	 * Number of allocated pages.
	 *
	 * The actual number of symbols and bytes per page depends on the current
	 * implementation. But we always can ensure that the actual number of
	 * allocated bytes for data, defined_and_merged and relative is always
	 * multiple of this number.
	 */
	unsigned int allocated_pages;

	/**
	 * Number of word at the beginning of the data that are not currently in use.
	 */
	unsigned int top;

	/**
	 * Actual data in the stack.
	 *
	 * This data should not be read without checking if the correspondign bytes are defined or not.
	 */
	unsigned char *data;

	/**
	 * Packed data of 2 bits each, containing if the corresponding byte in the
	 * data is defined (bit 0) and if the value comes from last merge of stacks (bit 1).
	 */
	packed_data_t *defined_and_merged;

	/**
	 * Whether the corresponding word value in the stack data is actually a
	 * value relative to the intial CS assigned by the OS.
	 *
	 * In the current implementation, we are assuming that only aligned words
	 * can be relative. Because of this, there is 1 bit here for every 2 bytes
	 * in the data.
	 */
	packed_data_t *relative;
};

/**
 * Initialize the stack structure as an empty stack.
 *
 * This is assuming that there was no a valid stack initialised before, and it
 * will not try to free any memory. If there was a valid stack, use clear_stack
 * method instead.
 */
void initialize_stack(struct Stack *stack);

/**
 * Clear an existing stack, freeing any pointer and setting the empty stack again.
 *
 * This assumes that there was a valid stack initialized in the structure already.
 * If that is not the case, use initialize_stack method instead.
 */
void clear_stack(struct Stack *stack);

/**
 * Check if the value in the top of this stack is defined and have a valid value that can be retrieved.
 *
 * This will return 0 if the stack is empty.
 */
int top_is_defined_in_stack(const struct Stack *stack);

/**
 * Check if the value in the top of this stack is defined and NO relative to the initial CS.
 *
 * This will return 0 if the stack is empty.
 */
int top_is_defined_absolute_in_stack(const struct Stack *stack);

/**
 * Check if the value in the top of this stack is both defined and relative to the initial CS.
 *
 * This will return 0 if the stack is empty.
 */
int top_is_defined_relative_in_stack(const struct Stack *stack);

/**
 * Check if the word at the given index starting from the top is defined in this stack.
 *
 * Calling this with index 0 is exactly the same as calling top_is_defined_in_stack method.
 *
 * In case the given index points outside the stack, this method will return 0.
 */
int is_defined_in_stack_from_top(const struct Stack *stack, unsigned int index);

/**
 * Check if the word at the given index starting from the top is defined and NOT relative to the initial CS in this stack.
 *
 * Calling this with index 0 is exactly the same as calling top_is_defined_absolute_in_stack method.
 *
 * In case the given index points outside the stack, this method will return 0.
 */
int is_defined_absolute_in_stack_from_top(const struct Stack *stack, unsigned int count);

/**
 * Check if the word at the given index starting from the top is both defined and relative to the initial CS in this stack.
 *
 * Calling this with index 0 is exactly the same as calling top_is_defined_relative_in_stack method.
 *
 * In case the given index points outside the stack, this method will return 0.
 */
int is_defined_relative_in_stack_from_top(const struct Stack *stack, unsigned int index);

/**
 * Get the word value at the given index from the top. This method does not alter the stack.
 */
uint16_t get_from_top(const struct Stack *stack, unsigned int index);

/**
 * Push an undefined word value in the stack.
 *
 * This will move the top of the stack one position and will enlarge the memory reserved for the stack when required.
 *
 * This will return something different from 0 in case of error, mainly cause when requesting for memory.
 */
int push_undefined_in_stack(struct Stack *stack);

/**
 * Push the given word value in the stack.
 *
 * This will move the top of the stack one position and will enlarge the memory reserved for the stack when required.
 *
 * This will return something different from 0 in case of error, mainly cause when requesting for memory.
 */
int push_in_stack(struct Stack *stack, uint16_t value);

/**
 * Push the given word value in the stack, assuming that the value is relative from the initial CS.
 *
 * This will move the top of the stack one position and will enlarge the memory reserved for the stack when required.
 *
 * This will return something different from 0 in case of error, mainly cause when requesting for memory.
 */
int push_relative_in_stack(struct Stack *stack, uint16_t value);

/**
 * This returns the word value in the top of the stack, and move the top one position.
 *
 * This will return the value in the stack, does not matter is it is defined or not.
 * Caller should call top_is_defined_in_stack or top_is_defined_relative_in_stack
 * before to know the nature of this value.
 *
 * In case the stack is empty, this method will return 0 and the top will not be moved.
 */
uint16_t pop_from_stack(struct Stack *stack);

/**
 * Replace the byte value located at (top * 2 + offset) within the stack.
 * This method will no move the top of this stack.
 *
 * Note that top refers to word values (16bit integers), while this method is
 * about bytes. This is why top has to be multiplied by 2 before being used internally.
 */
int set_byte_in_stack_from_top(struct Stack *stack, unsigned int offset, unsigned char value);

/**
 * Replace the word value located at byte position (top * 2 + offset) within the stack.
 * This method will no move the top of this stack.
 *
 * Note that top refers to word values (16bit integers), while offset parameter here refers to byte position.
 * This is why top has to be multiplied by 2 before being used internally.
 */
int set_word_in_stack_from_top(struct Stack *stack, unsigned int offset, uint16_t value);

/**
 * Replace the word value located at byte position (top * 2 + offset) within the stack.
 * This assumes that the value given is relative to the initial CS and will be marked like this internally.
 * This method will no move the top of this stack.
 *
 * Note that top refers to word values (16bit integers), while offset parameter here refers to byte position.
 * This is why top has to be multiplied by 2 before being used internally.
 */
int set_relative_word_in_stack_from_top(struct Stack *stack, unsigned int offset, uint16_t value);

/**
 * Copy the contents in the source stack into the target stack.
 * This will free any unrequired data in the target stack.
 */
int copy_stack(struct Stack *target_stack, const struct Stack *source_stack);

/**
 * Creates a new stack as a result of merging the given 2 stacks.
 * The result will be placed in the first stack parameter.
 *
 * This will iterate in both stacks from their tops comparing the values.
 * Only if the value is defined and the same in both stacks positions, the
 * resulting stack will include it.
 *
 * If the value is unknown in any of the stacks, or the values do not match,
 * then it will result in an unknown value for that position.
 *
 * In case any of the values was known, or marked as merged, the result will be
 * marked as merged in the resulting stack, meaning that to source stack can be
 * checked in backtracing in order to get the different possible values.
 */
int merge_stacks(struct Stack *stack, const struct Stack *other_stack);

/**
 * Returns something different from 0 if we know that merging both stack using
 * the merge_stacks method will result in a different stack than the given stack.
 *
 * This method can be used to check if something changed from the last merge, and act in consequence.
 */
int changes_on_merging_stacks(const struct Stack *stack, const struct Stack *other_stack);

#ifdef DEBUG
void print_stack(const struct Stack *stack);
#endif /* DEBUG */

#endif /* _STACK_H_ */
