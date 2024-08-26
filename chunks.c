#include "include.h"
#include <sys/mman.h>

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;

Block *find_free_block(Block **last, size_t size, size_t alignment) {
    Block *current = freelist;
    while (current && !(current->free && current->size >= size)) 
	{
        *last = current;
        current = current->next;
    }
    return current;
}


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
			printf("Splitting block at %p into blocks of size %zu and %zu\n", block, block->size, 0);
		#endif
	}
}

Block *request_space_sbrk(Block *last, size_t size, size_t alignment) {
    Block *block = sbrk(0);
	#ifdef DEBUG
		printf("Requesting space with sbrk\n");
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
		printf("\n");
	#endif
    return block;
}

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

    void *aligned_addr = (void *)ALIGN((uintptr_t)(block + 1), alignment);
    block->aligned_address = aligned_addr;

    allocated_blocks++;
	#ifdef DEBUG
		printf("Allocated memory at address %p\n", block->aligned_address);
	    printf("Allocated blocks: %d\n", allocated_blocks);	
		printf("\n");
	#endif
    return block->aligned_address;
}
