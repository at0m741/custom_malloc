#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;

/*
	* Function to coalesce free blocks
	* this function is called after freeing a block
	* it coalesces adjacent free blocks
	* if two adjacent blocks are free, they are merged into a single block
	* this is done to reduce fragmentation
	* the size of the first block is increased by the size of the second block
	* the next pointer of the first block is updated to point to the block after the second block
	* this process is repeated until no more free blocks can be coalesced
	* this function is called after freeing a block
*/


__attribute__((hot, flatten, always_inline))
inline void coalesce_free_blocks() {
    Block *current = freelist;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;

            uintptr_t aligned_addr = (uintptr_t)(current + 1);
            if ((aligned_addr & ALIGNMENT - 1) != 0) 
                printf("Warning: Coalesced block not aligned at %p\n", (void *)aligned_addr);
		_mm_prefetch(current->next, _MM_HINT_T0);
        } else {
            current = current->next;
        }
    }
}

/*
	* Function to free a block of memory
	* ptr: pointer to the block to be freed
	* this function is called to free a block of memory
	* if the block was allocated using mmap, it is freed using munmap
	* if the block was allocated using sbrk, it is marked as free
	* the block is added to the freelist
	* the free flag is set to 1
	* the number of freed blocks is incremented
*/


__attribute__((hot, always_inline))
inline void _free(void *ptr) 
{
    if (!ptr)
        return;
    Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
	if (ptr != block->aligned_address) 
		ptr = block->aligned_address;	

    if (__builtin_expect(block->size > block_size, 0)) 
	{
        size_t alignment_mask = sysconf(_SC_PAGESIZE) - 1;
        size_t total_size = block->size + sizeof(Block) + alignment_mask;
		total_size = __builtin_align_up(total_size, ALIGNMENT);
        munmap((void *)block, total_size);
    } else {
        block->free = 1;
		coalesce_free_blocks();
    }
    allocated_blocks--;
	block->free = 0;
}

