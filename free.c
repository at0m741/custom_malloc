#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;

void coalesce_free_blocks() {
    Block *current = freelist;
    while (current && current->next) 
	{
        if (current->free && current->next->free) 
		{
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
        } 
		else
            current = current->next;
		#ifdef DEBUG
			printf("Coalescing free blocks\n");
			printf("Block at %p has size %zu\n", current, current->size);
		#endif
	}
	#ifdef DEBUG
		printf("\n");
	#endif
}

void _free(void *ptr) {
    if (__builtin_expect(ptr == NULL, 0))
		return;

    Block *block = (Block *)ptr - 1;
    block->free = 1;
    freed_blocks++;
	#ifdef DEBUG
		printf("Freeing block at %p\n", block);
	#endif
    coalesce_free_blocks();
}
