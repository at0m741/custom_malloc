#include "include.h"


extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;


int main() {
    int *ptr1 = (int *)_malloc(sizeof(int) * 10);
    int *ptr2 = (int *)_malloc(sizeof(int) * 20);
    int *ptr3 = (int *)_malloc(sizeof(int) * 30);
	int *ptr4 = (int *)_malloc(0);

    if (ptr1 == NULL || ptr2 == NULL || ptr3 == NULL) {
        printf("Allocation failed\n");
        return 1;
    }

    printf("Number of blocks allocated: %d\n", allocated_blocks);

    get_cache_info();
    printf("Allocated memory at address %p (Block 1)\n", ptr1);
    printf("Allocated memory at address %p (Block 2)\n", ptr2);
    printf("Allocated memory at address %p (Block 3)\n", ptr3);
    printf("\n");

    for (int i = 0; i < 10; i++)
        ptr1[i] = i;
    for (int i = 0; i < 20; i++)
        ptr2[i] = i + 10;
    for (int i = 0; i < 30; i++)
        ptr3[i] = i + 30;

    hexdump(ptr1, sizeof(int) * 10);
    printf("\n");
    hexdump(ptr2, sizeof(int) * 20);
    printf("\n");
    hexdump(ptr3, sizeof(int) * 30);
    printf("\n");

    size_t large_size = 200 * 1024;
    int *large_ptr = (int *)_malloc(large_size);

    if (large_ptr == NULL) {
        printf("Large allocation failed\n");
        return 1;
    }

    printf("Number of blocks allocated after large allocation: %d\n", allocated_blocks);

   // allocate_cache(large_size);
    printf("Allocate large memory at address %p (Large Block, mmap)\n", large_ptr);

    for (int i = 0; i < large_size / sizeof(int); i++)
        large_ptr[i] = i % 100;

    hexdump(large_ptr, large_size / 1024);

    _free(ptr1);
    _free(ptr2);
    _free(ptr3);
    _free(large_ptr);
    printf("Number of blocks freed: %d\n", freed_blocks);

    check_for_leaks();    
    return 0;}
