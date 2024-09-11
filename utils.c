#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;


/* void log_block(Block *block) { */
/*    (void)block;  */
/* } */

int count_blocks(Block *list) {
	Block *current = list;
	int count = 0;
	while (current != NULL) {
		count++;
		current = current->next;
	}
	return count;
}

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
    for (size_t i = 0; i < size; i += 16) {
        printf("%p: ", p + i);

        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", p[i + j]);
        }

        for (size_t j = size - i < 16 ? 16 - (size - i) : 0; j > 0; j--) {
            printf("   ");
        }

        printf(" |");
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            unsigned char c = p[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
	printf("\n");
	printf("\n");
	printf("Size of the memory block: %lu bytes\n", size);
	printf("Address of the memory block: %p\n", ptr);
	printf("Number of blocks allocated: %d\n", count_blocks(freelist));

	printf("\n");
}
