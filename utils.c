#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;

void check_for_leaks() {
    if (allocated_blocks != freed_blocks) 
        printf("Potential memory leak detected: %d blocks allocated, %d blocks freed.\n",
               allocated_blocks, freed_blocks);
    else
        printf("No memory leaks detected.\n");
}

void check_alignment(void *ptr) {
	if ((uintptr_t)ptr % 16 != 0)
		printf("Memory not aligned to 16 bytes\n");
	else
		printf("Memory aligned to 16 bytes\n");
}

void hexdump(void *ptr, size_t size) {
    unsigned char *p = (unsigned char *)ptr;
    printf("%p: ", p);
    for (size_t i = 0; i < size; i++) 
	{
        printf("%02x ", p[i]);
        if ((i + 1) % 16 == 0)
            printf("\n%p: ", p + i + 1);
    }
	printf("\n");
}
