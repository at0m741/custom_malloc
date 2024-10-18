#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;
extern MemoryAllocator allocator;

void heap_info(void) {
    static long nb_call = 0;
    long total_alloc = 0;


    printf("Heap Info #%ld:\n", nb_call);
    
    for (Block *heap = freelist; heap; heap = heap->next) {
        printf("Heap: %p - %p\n", (void *)heap, (void *)((char *)heap + heap->size));
        printf("  Size: %zu\n", heap->size);
        printf("  Free space: %d\n", heap->free);
		printf("  Aligned address: %p\n", heap->aligned_address);
		printf("  Is mmap: %d\n", heap->is_mmap);
        
        if (heap->free) {
            printf("  Freed: yes\n");
        } else {
            printf("  Freed: no\n");
            total_alloc += heap->size; 
        }
    }

    printf("Total allocated memory: %ld bytes\n", total_alloc);

    nb_call++;
}

size_t print_blocks(Block *block) {
    size_t total_alloc_block = 0;
    while (block) {
        printf("%p - %p : %zu bytes\n", 
               block, 
               (void *)block + block->size, 
               block->size);
        
        if (!block->free) {
            total_alloc_block += block->size;
        }
        block = block->next;
    }
    return total_alloc_block;
}

int count_blocks(Block *list) 
{
	Block *current = list;
	int count = 0;
	while (current != NULL) {
		count++;
		current = current->next;
	}
	printf("Number of blocks: %d\n", count);
	heap_info();
	print_blocks(list);
	return count;
}

void check_for_leaks() 
{
	MemoryAllocator *alloc = NULL;
    if (allocated_blocks != freed_blocks) 
        printf("Potential memory leak detected: %d blocks allocated, %d blocks freed.\n",
               allocated_blocks, freed_blocks);
    else
        printf("No memory leaks detected.\n");
}

void check_alignment(void *ptr) 
{
	if ((uintptr_t)ptr % 16 != 0)
		printf("Memory not aligned to 16 bytes\n");
	else
		printf("Memory aligned to 16 bytes\n");
}

void hexdump(void *ptr, size_t size) 
{
    unsigned char *p = (unsigned char *)ptr;
    for (size_t i = 0; i < size; i += 16) {
        printf("%p: ", p + i);

		for (size_t j = 0; j < 16 && i + j < size; j++) 
			printf("%02x ", p[i + j]);

		for (size_t j = size - i < 16 ? 16 - (size - i) : 0; j > 0; j--)
            printf("   ");

        printf(" |");
        for (size_t j = 0; j < 16 && i + j < size; j++) 
		{
            unsigned char c = p[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
	printf("\n");
}
