#include "include.h"
#include <sys/mman.h>

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;

/*
* Function to find a free block in the freelist
* size: size of the memory to be allocated
* alignment: alignment of the memory to be allocated
* Returns: pointer to the allocated memory
*/

Block *find_free_block(Block **last, size_t size, size_t alignment) {
    Block *current = freelist;
    while (current && !(current->free && current->size >= size)) 
	{
        *last = current;
        current = current->next;
		#ifdef DEBUG
			printf("Searching for free block\n");
			printf("Current block: %p\n", current);
			printf("\n");
		#endif
    }
    return current;
}

/*
	* Function to split a block into two blocks
* block: block to be split
* size: size of the memory to be allocated
* this function is called when the block is larger than the requested size
* the block is split into two blocks, one of the requested size and the other of the remaining size
*/


void split_block(Block *block, size_t size) {
	if (block->size > size + BLOCK_SIZE) 
	{
		Block *new_block = (Block *)((char *)block + BLOCK_SIZE + size);
		new_block->size = block->size - size - BLOCK_SIZE;
		new_block->free = 1;
		new_block->next = block->next;
		block->size = size;
		block->next = new_block;
		#ifdef DEBUG
			printf("Splitting block at %p into blocks of size %zu and %zu\n", block, block->size, new_block->size);
		#endif
	}
	else 
	{
		#ifdef DEBUG
			printf("Splitting block at %p into blocks of size %zu and %d\n", block, block->size, 0);
			printf("Block size is less than requested size\n");
			printf("\n");
		#endif
	}
}

/*
* this function call sbrk to allocate memory
* last: last block in the freelist
* size: size of the memory to be allocated
* alignment: alignment of the memory to be allocated
* only be used if the requested size is larger than the block size
* Returns: pointer to the allocated memory
*/

Block *request_space_sbrk(Block *last, size_t size, size_t alignment) {
    Block *block = sbrk(0);
	#ifdef DEBUG
		printf("Requesting space with sbrk\n");
		printf("Requested size: %zu\n", size);
	#endif
    void *request = sbrk(size + BLOCK_SIZE + alignment); 
    if (request == (void *)-1) 
        return NULL;
    
    block->size = size;
    block->next = NULL;
    block->free = 0;
    void *aligned_addr = (void *)ALIGN((uintptr_t)(block + 1), alignment);
    block->aligned_address = aligned_addr;

    if (last)
        last->next = block;

    allocated_blocks++;
	#ifdef DEBUG
		printf("Allocated memory at address %p\n", block->aligned_address);
	    printf("Allocated blocks: %d\n", allocated_blocks);
		check_alignment(block->aligned_address);
		printf("\n");
	#endif
    return block;
}

/*
* this function call mmap to allocate memory
* size: size of the memory to be allocated
* alignment: alignment of the memory to be allocated
* Returns: pointer to the allocated memory
*/


void *request_space_mmap(size_t size, size_t alignment) {
    size_t total_size = size + BLOCK_SIZE + alignment;
	#ifdef DEBUG
		printf("Requesting space with mmap\n");
	#endif
    void *mapped_memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mapped_memory == MAP_FAILED)
        return NULL;

    Block *block = (Block *)mapped_memory;
    block->size = size;
    block->next = NULL;
    block->free = 0;
	block->is_mmap = 1;

    void *aligned_addr = (void *)ALIGN((uintptr_t)(block + 1), alignment);
    block->aligned_address = aligned_addr;

    allocated_blocks++;
	#ifdef DEBUG
		printf("Allocated memory at address %p\n", block->aligned_address);
	    printf("Allocated blocks: %d\n", allocated_blocks);	
		check_alignment(block->aligned_address);
		printf("\n");
	#endif
    return block->aligned_address;
}

/*
 * function to binary search for a block in the freelist
 * ptr: pointer to the memory block
 * Returns: pointer to the block
 */

Block *binary_search(Block *ptr) {
	Block *current = freelist;
	Block *last = NULL;
	while (current != NULL) 
	{
		if (current == ptr) 
		{
			#ifdef DEBUG
				printf("Block found\n");
				printf("Block address: %p\n", current);
				printf("\n");
			#endif
			return current;
		}
		last = current;
		current = current->next;
	}
	#ifdef DEBUG
		printf("Block not found\n");
		printf("\n");
	#endif
	return NULL;
}
