#ifndef _CODE_BLOCKS_H_
#define _CODE_BLOCKS_H_

#include <stdlib.h>

/**
 * Structure reflecting a piece of code that is always executed sequentially, except due to conditional jumps or interruptions.
 */
struct CodeBlock {
	unsigned int relative_cs;
	unsigned int ip;
	const char *start;

	/**
	 * This should always be equals or greater than start.
	 * This is initially set to match start. If so, it means that this block has not been read yet,
	 * and then we do not knwo for sure where it ends. Once the block is read, end will be placed
	 * to the first position outside the block, which may match with the start of another block or not.
	 */
	const char *end;
};

/**
 * Complex structure containing pages of code blocks.
 * It is designed to grow as more blocks are added to it.
 */
struct CodeBlockList {
	/**
	 * Reflects that the page_array allocated size must be always a multiple of this value.
	 * The bigger, the more we will avoid calling realloc for each new inserted block, improving its performance.
	 * The smaller, the less memory this list will use.
	 * This parameter should be adjusted according to the environtment where this executable should run.
	 */
	unsigned int page_array_granularity;

	/**
	 * Number of blocks in each page.
	 * The bigger, the more we will avoid calling realloc for each new inserted block and the less
	 * the page_array will grow, improving its performance.
	 * This parameter should be adjusted according to the environtment where this executable should run.
	 */
	unsigned int blocks_per_page;

	/**
	 * Array holding all allocated pages in the order they have been allocated.
	 * The allocated size of this array can be calculated combining the values in block_count,
	 * blocks_per_page and page_array_granularity.
	 * This will be NULL when block_count is 0.
	 */
	struct CodeBlock **page_array;

	/**
	 * Array pointing to all code blocks, sorted by its start.
	 * The allocated size of this array can be calculated combining the values in block_count,
	 * blocks_per_page and page_array_granularity.
	 * This will be NULL when block_count is 0.
	 */
	struct CodeBlock **sorted_blocks;

	/**
	 * Number of blocks already inserted.
	 * Note that this value is most of the times lower than the actual capacity allocated in memory
	 * to hold all of them.
	 */
	unsigned int block_count;
};

/**
 * Set all its values. After this, this list will be empty, but ready.
 */
void initialize_code_block_list(struct CodeBlockList *list);

/**
 * Searches for a block whose start matches the given one.
 * This will return the index of the block within the sorted_blocks array. Or -1 if none matches.
 */
int index_of_code_block_with_start(const struct CodeBlockList *list, const char *start);

/**
 * Searches for a block whose start matches he given position.
 * If none matches, this method will return the block whose start is closest but before the given position.
 * This will return -1 if there are not blocks, or all of them has a start greater than the given position.
 * This will return the index of the block within the sorted_blocks array.
 */
int index_of_code_block_containing_position(const struct CodeBlockList *list, const char *position);

/**
 * Returns a proper pointer to store a new block.
 * The returned pointer should be filled and call insert_sorted_code_block method in order to sort it properly.
 * This method may require allocating a new page of memory.
 * This method will return NULL in case of failure.
 */
struct CodeBlock *prepare_new_code_block(struct CodeBlockList *list);

/**
 * Inserts the code block returned previously by prepare_new_code_block.
 * This will increase the block_count value by 1 and will update the sorted_blocks inserting the
 * reference to this block in the suitable position.
 */
int insert_sorted_code_block(struct CodeBlockList *list, struct CodeBlock *new_block);

/**
 * Free all the allocated block and restores this list to its initial state.
 */
void clear_code_block_list(struct CodeBlockList *list);

#endif // _CODE_BLOCKS_H_