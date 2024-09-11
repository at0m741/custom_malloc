#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;

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

#define MAX_BLOCK_SIZE 1024 * 1024
#include <assert.h>

void coalesce_free_blocks() {
    Block *current = freelist;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + BLOCK_SIZE;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }

	#ifdef DEBUG
		printf("Coalescing free blocks\n");
		printf("size of block: %zu\n", current->size);
		printf("\n");
	#endif
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
void log_block(Block *block) {
    if (!block) {
        printf("Erreur: tentative d'accès à un bloc NULL\n");
        return;
    }

    // Vérifiez la taille du bloc avant d'y accéder
    if (block->size == 0 || block->size > MAX_BLOCK_SIZE) {
        printf("Erreur: bloc corrompu ou taille invalide à l'adresse %p\n", block);
        return;
    }

    printf("Block at %p: size = %zu, free = %d\n", block, block->size, block->free);
}

void _free(void *ptr) {
    if (__builtin_expect(ptr == NULL, 0)){
        return;
    }

    Block *block = (Block *)((char *)ptr - sizeof(Block));
    assert(block != NULL);

    if (block->free) {
        fprintf(stderr, "Erreur: tentative de libérer un bloc déjà libéré à l'adresse %p\n", block);
        return;
    }

    log_block(block);

    if (block->is_mmap) {
        munmap(block, block->size + sizeof(Block));
        allocated_blocks--;
        freed_blocks++;
        #ifdef DEBUG
            printf("Freeing mmap block at %p\n", block);
            printf("Allocated blocks: %d\n", allocated_blocks);
            printf("Size of block: %zu\n", block->size);
            printf("\n");
        #endif
        return;  // Sortir après munmap
    }

    // Sbrk block
    printf("Size of block: %zu\n", block->size);
    block->free = 1;
    freed_blocks++;
    
    #ifdef DEBUG
        printf("free then coalesce_free_blocks\n");
        printf("Block at %p has size %zu\n", block, block->size);
        printf("Freeing block at %p\n", block);
        printf("\n");
    #endif

    coalesce_free_blocks();
    log_block(block);
}
