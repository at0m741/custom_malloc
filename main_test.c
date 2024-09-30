#include "include.h"


extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;

int ft_strlen(const char *s) {
	int len = 0; 
	while (s[len] != '\0') {
		len++;
	}
	return len;
}

char *ft_strdup(const char *s) {
	size_t len = ft_strlen(s);
	char *new = (char *)_malloc(len + 1);
	if (new == NULL)
		return NULL;
	_memcpy_ERMS(new, s, len);
	new[len] = '\0';
	hexdump(new, len + 1);
	return new;
}

#include <time.h>

int main() {
	printf("-------------------- malloc --------------------\n");
    int *ptr1 = (int *)_malloc(sizeof(int) * 10);
    int *ptr2 = (int *)_malloc(sizeof(int) * 20);
    int *ptr3 = (int *)_malloc(sizeof(int) * 30);

    /* if (ptr1 == NULL || ptr2 == NULL || ptr3 == NULL) { */
    /*     printf("Allocation failed\n"); */
    /*     return 1; */
    /* } */

    printf("Number of blocks allocated: %d\n", allocated_blocks);

    get_cache_info();
    printf("Allocated memory at address %p (Block 1)\n", ptr1);
    printf("Allocated memory at address %p (Block 2)\n", ptr2);
    printf("Allocated memory at address %p (Block 3)\n", ptr3);
    printf("\n");
	if (ptr1 == NULL || ptr2 == NULL || ptr3 == NULL) {
		printf("Allocation failed\n");
		return 1;
	}
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
    printf("Allocate large memory at address %p (Large Block, mmap)\n", large_ptr);

    for (int i = 0; i < large_size / sizeof(int); i++)
        large_ptr[i] = i % 100;

    hexdump(large_ptr, large_size / 1024);
	printf("\n");
	

	printf("-------------------- aligned_alloc (taille non multiple) --------------------\n");
	void *ptr_aligned_non_multiple = _aligned_alloc(32, 18); 
	if (ptr_aligned_non_multiple == NULL) {
		printf("Allocation failed\n");
	} else {
		printf("Aligned memory at address %p\n", ptr_aligned_non_multiple);
		hexdump(ptr_aligned_non_multiple, 18);
		_free(ptr_aligned_non_multiple);
	}
	/* printf("-------------------- intentional memory leak --------------------\n"); */
	/* int *leak_ptr = (int *)_malloc(sizeof(int) * 5); */
	/* // Intentionally not freeing leak_ptr to simulate a memory leak */
	/* check_for_leaks(); */
	/* _free(leak_ptr); */
	printf("\n");
	printf("-------------------- aligned_alloc --------------------\n");
	int *ptr5 = (int *)_aligned_alloc(32, sizeof(int) * 10);
	if (ptr5 == NULL) {
		printf("Allocation failed\n");
		return 1;
	}
	hexdump(ptr5, sizeof(int) * 10);
	printf("Allocated memory at address %p (Block 5)\n", ptr5);
	
	void *ptr6 = (void *)_aligned_alloc(16, 100);
	if (ptr6 == NULL) {
		printf("Allocation failed\n");
		return 1;
	}
	hexdump(ptr6, 100);
	printf("Allocated memory at address %p (Block 6)\n", ptr6);

	printf("-------------------- free --------------------\n");
    _free(ptr1);
    _free(ptr2);
    _free(ptr3);
	printf("-------------------- free (large) --------------------\n");
    _free(large_ptr);
	printf("-------------------- free (aligned_alloc) --------------------\n");
	_free(ptr5);
	_free(ptr6);
	printf("Number of blocks freed: %d\n", freed_blocks);
	printf("\n");
	
	printf("-------------------- check_for_leaks --------------------\n");
	check_for_leaks();
	printf("\n");
	
	printf("-------------------- aligned_alloc --------------------\n");
	void *ptr7 = (void *)_aligned_alloc(32, 100000000 * sizeof(int));
	if (ptr7 == NULL) {
		printf("Allocation failed\n");
		return 1;
	}
	for (int i = 0; i < 100; i++)
		((int *)ptr7)[i] = i;
	hexdump(ptr7, 100);
	printf("Allocated memory at address %p (Block 7)\n", ptr7);
	
	printf("-------------------- free (aligned_alloc) --------------------\n");
	_free(ptr7);
	printf("Number of blocks freed: %d\n", freed_blocks);
	printf("\n");
	
	printf("-------------------- check_for_leaks --------------------\n");
    check_for_leaks();   
	
	printf("-------------------- ft_strdup --------------------\n");
	char *str = "Hello, World! from ft_strdup with my own implementation of malloc and free";

	char *dup_str = ft_strdup(str);
	printf("len: %d\n", ft_strlen(dup_str));
	_free(dup_str);

	printf("-------------------- benchmark --------------------\n");
	
	clock_t start, end;
	double cpu_time_used;
	start = clock();
	int *ptr8 = (int *)_malloc(sizeof(int) * 1000000);
	for (int i = 0; i < 1000000; i++)
		ptr8[i] = i;
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Time taken to write 1,000,000 integers using custom malloc: %f seconds\n", cpu_time_used);
	printf("number of blocks allocated: %d\n", allocated_blocks);
	_free(ptr8);

	start = clock();
	int *ptr9 = (int *)malloc(sizeof(int) * 1000000);
	for (int i = 0; i < 1000000; i++)
		ptr9[i] = i;
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Time taken to write 1,000,000 integers using malloc: %f seconds\n", cpu_time_used);
	printf("number of blocks allocated: %d\n", allocated_blocks);
	free(ptr9);
	
	start = clock();
	int *ptraligned = (int *)_aligned_alloc(32, sizeof(int) * 1000000);
	for (int i = 0; i < 1000000; i++)
		ptraligned[i] = i;
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Time taken to write 1,000,000 integers using custom aligned_alloc: %f seconds\n", cpu_time_used);
	_free(ptraligned);
	check_for_leaks();

	start = clock();
	for (int i = 0; i < 1000; i++)
	{
		int *ptr10 = (int *)_malloc(sizeof(int));
		*ptr10 = i;
		_free(ptr10);
	}
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Time taken to write 1,000,000 integers using malloc and allocating 1 byte at a time: %f seconds\n", cpu_time_used);
	check_for_leaks();
	hexdump(freelist, 1000);
	printf("-------------------- end --------------------\n");
	printf("Number of blocks allocated: %d\n", allocated_blocks);
	printf("Number of blocks freed: %d\n", freed_blocks);


    return 0;
}
