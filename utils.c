#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern MemoryAllocator allocator;

#include <stdio.h>

// Définition des codes de couleurs ANSI
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define BOLD "\033[1m"

void heap_info(void) {
    static long nb_call = 0;
    long total_alloc = 0;

    printf(BOLD CYAN "Heap Info #%ld:\n" RESET, nb_call);

    for (Block *heap = freelist; heap; heap = heap->next) {
        printf(BOLD BLUE "Heap: %p - %p\n" RESET, (void *)heap, (void *)((char *)heap + heap->size));
        printf("  " YELLOW "Size: " RESET "%zu\n", heap->size);
        printf("  " YELLOW "Free space: " RESET "%d\n", heap->free);
        printf("  " YELLOW "Aligned address: " RESET "%p\n", heap->aligned_address);
        printf("  " YELLOW "Is mmap: " RESET "%d\n", heap->is_mmap);

        if (allocated_blocks == 0) {
            printf("  " GREEN "Freed: yes\n" RESET);
        } else {
            printf("  " RED "Freed: no\n" RESET);
            total_alloc += heap->size;
			printf(RED "total_alloc %ld\n" RED, total_alloc);
        }
    }

    printf(BOLD CYAN "Total allocated memory: %ld bytes\n" RESET, total_alloc);

    nb_call++;
}

size_t print_blocks(Block *block) {
    size_t total_alloc_block = 0;
    while (block) {
        printf(BLUE "%p - %p" RESET " : " YELLOW "%zu bytes\n" RESET, 
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
    printf(CYAN "Number of blocks: " RESET "%d\n", count);
    heap_info();
    print_blocks(list);
    return count;
}

void check_for_leaks() 
{
    if (allocated_blocks != 0) 
        printf(RED "Potential memory leak detected: " RESET "%d blocks allocated, %d blocks freed.\n",
               allocated_blocks, 0);
    else
        printf(GREEN "No memory leaks detected.\n" RESET);
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
