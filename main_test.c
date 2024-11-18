#include "include.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

extern Block *freelist;
extern int allocated_blocks;

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
    return new;
}
char *ft_itoa(int n) {
	int len = 0;
	int temp = n;
	while (temp != 0) {
		temp /= 10;
		len++;
	}
	char *str = (char *)_malloc(len + 1);
	if (str == NULL)
		return NULL;
	str[len] = '\0';
	while (n != 0) {
		str[--len] = n % 10 + '0';
		n /= 10;
	}
	return str;
}

#define NUM_SMALL_ALLOCS 1000
#define NUM_LARGE_ALLOCS 100
#define MAX_ALLOCATION_SIZE (1 << 20)

void test_random_alloc_free() {
    printf("\n== Random Alloc/Free Test ==\n");
    srand((unsigned int)time(NULL));
    void *allocations[NUM_SMALL_ALLOCS];
    memset(allocations, 0, sizeof(allocations));

    for (size_t i = 0; i < NUM_SMALL_ALLOCS * 10; i++) {
        size_t index = rand() % NUM_SMALL_ALLOCS;
        if (allocations[index]) {
            _free(allocations[index]);
            allocations[index] = NULL;
        } else {
            size_t size = (rand() % 256) + 1;
            allocations[index] = _malloc(size);
            if (!allocations[index]) {
                fprintf(stderr, "Error: Random allocation failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (size_t i = 0; i < NUM_SMALL_ALLOCS; i++) {
        if (allocations[i]) {
            _free(allocations[i]);
        }
    }

    printf("Random alloc/free test passed.\n");
}

void test_small_allocations() {
    printf("\n== Small Allocations Test ==\n");
    void *allocations[NUM_SMALL_ALLOCS];
    for (size_t i = 0; i < NUM_SMALL_ALLOCS; i++) {
        size_t size = (i % 64) + 1; 
        allocations[i] = _malloc(size);
        if (!allocations[i]) {
            fprintf(stderr, "Error: Failed to allocate %zu bytes\n", size);
            exit(EXIT_FAILURE);
        }
        memset(allocations[i], 0xAA, size); 
    }

    for (size_t i = 0; i < NUM_SMALL_ALLOCS; i++) {
        _free(allocations[i]);
    }
    printf("Small allocations test passed.\n");
}

void test_large_allocations() {
    printf("\n== Large Allocations Test ==\n");
    void *allocations[NUM_LARGE_ALLOCS];
    for (size_t i = 0; i < NUM_LARGE_ALLOCS; i++) {
        size_t size = 1024 * (i + 1); 
        allocations[i] = _malloc(size);
        if (!allocations[i]) {
            fprintf(stderr, "Error: Failed to allocate %zu bytes\n", size);
            exit(EXIT_FAILURE);
        }
        memset(allocations[i], 0xBB, size); 
    }

    for (size_t i = 0; i < NUM_LARGE_ALLOCS; i++) {
        _free(allocations[i]);
    }
    printf("Large allocations test passed.\n");
}


void test_alignment() {
    size_t alignments[] = {16, 32, 64, 128, 256, 512, 1024};
    size_t sizes[] = {128, 256, 512, 1024};

    for (size_t i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
        for (size_t j = 0; j < sizeof(sizes) / sizeof(sizes[0]); j++) {
            void *ptr = _malloc(sizes[j]);
            if ((uintptr_t)ptr % alignments[i] != 0) {
                printf("Error: Pointer %p is not aligned to %zu bytes\n", ptr, alignments[i]);
            } else {
                printf("Success: Pointer %p is aligned to %zu bytes\n", ptr, alignments[i]);
            }
            _free(ptr);
        }
    }
}


int is_in_mapped_heap(void *addr) {
    uintptr_t start, end;
    char line[256];
    FILE *maps = fopen("/proc/self/maps", "r");

    if (!maps) {
        perror("fopen");
        return 0;
    }

    uintptr_t addr_val = (uintptr_t)addr;
    int in_heap = 0;

    while (fgets(line, sizeof(line), maps)) {
        // Vérifie si la ligne représente un segment anonyme (mmap) en lecture et écriture
        if (strstr(line, "rw-p") && strstr(line, "00:00") && !strstr(line, "/")) { 
            sscanf(line, "%lx-%lx", &start, &end);
            if (addr_val >= start && addr_val < end) {
                in_heap = 1;
                break;
            }
        }
    }

    fclose(maps);
    return in_heap;
}

void check_address(void *addr) {
    if (is_in_mapped_heap(addr)) {
        printf("Oui, l'adresse %p est dans une région allouée via mmap.\n", addr);
    } else {
        printf("Non, l'adresse %p n'est pas dans une région allouée via mmap.\n", addr);
    }
}

void benchmark_malloc(size_t num_elements) {
    struct timespec start, end;
    double time_taken;

    printf("---- Benchmark with custom _malloc ----\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    int *ptr_custom = (int *)_malloc(sizeof(int) * num_elements);
    if (ptr_custom == NULL) {
        printf("Allocation failed for ptr_custom\n");
        return;
    }
    for (size_t i = 0; i < num_elements; i++)
        ptr_custom[i] = i;
    clock_gettime(CLOCK_MONOTONIC, &end);

    time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time taken with custom _malloc: %f seconds\n", time_taken);
    _free(ptr_custom);

    printf("---- Benchmark with standard malloc ----\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    int *ptr_standard = (int *)malloc(sizeof(int) * num_elements);
    if (ptr_standard == NULL) {
        printf("Allocation failed for ptr_standard\n");
        return;
    }
    for (size_t i = 0; i < num_elements; i++)
        ptr_standard[i] = i;
    clock_gettime(CLOCK_MONOTONIC, &end);

    time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time taken with standard malloc: %f seconds\n", time_taken);
    free(ptr_standard);
}

int main() {
    printf("===== Testing Custom Memory Allocator =====\n\n");

    printf("---- Simple Allocation Test ----\n");
    int *ptr1 = (int *)_malloc(sizeof(int) * 10);
    if (ptr1 == NULL) {
        printf("Allocation failed for ptr1\n");
        return 1;
    }
    for (int i = 0; i < 10; i++)
        ptr1[i] = i;
    printf("Allocation successful for ptr1 at address %p\n", ptr1);

    _free(ptr1);
    printf("Deallocation successful for ptr1\n\n");

    printf("---- ft_strdup Test ----\n");
    char *str = "Hello, World!";
    char *dup_str = ft_strdup(str);
    if (dup_str == NULL) {
        printf("String duplication failed\n");
        return 1;
    }
    printf("Duplicated string: %s\n", dup_str);
	hexdump(dup_str, ft_strlen(dup_str));
    _free(dup_str);
    printf("Deallocation successful for dup_str\n\n");

    printf("---- _aligned_alloc Test ----\n");
    int *ptr_aligned = (int *)_aligned_alloc(32, sizeof(int) * 20);
    if (ptr_aligned == NULL) {
        printf("Aligned allocation failed\n");
        return 1;
    }
    for (int i = 0; i < 20; i++)
        ptr_aligned[i] = i;
    printf("Aligned allocation successful at address %p\n", ptr_aligned);
	check_address(ptr_aligned);	
    if ((uintptr_t)ptr_aligned % 32 == 0)
        printf("Address is correctly aligned to 32 bytes\n");
    else
        printf("Address alignment error\n");
	hexdump(ptr_aligned, sizeof(int) * 20);
    _aligned_free(ptr_aligned);
    printf("Deallocation successful for ptr_aligned\n\n");

    printf("===== Performance Benchmark =====\n\n");
    const size_t num_elements = 1000000;
    clock_t start, end;
    double cpu_time_used;

    printf("---- Benchmark with custom _malloc ----\n");
    start = clock();
    int *ptr_custom = (int *)_malloc(sizeof(int) * num_elements);
    if (ptr_custom == NULL) {
        printf("Allocation failed for ptr_custom\n");
        return 1;
    }
    for (size_t i = 0; i < num_elements; i++)
        ptr_custom[i] = i;
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time taken with custom _malloc: %f seconds\n", cpu_time_used);
	check_address(ptr_custom);
    _free(ptr_custom);
    printf("Deallocation successful for ptr_custom\n\n");

    printf("---- Benchmark with standard malloc ----\n");
    start = clock();
    int *ptr_standard = (int *)malloc(sizeof(int) * num_elements);
	printf("allocated_blocks: %d\n", allocated_blocks);
    if (ptr_standard == NULL) {
        printf("Allocation failed for ptr_standard\n");
        return 1;
    }
    for (size_t i = 0; i < num_elements; i++)
        ptr_standard[i] = i;
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time taken with standard malloc: %f seconds\n", cpu_time_used);
	hexdump(ptr_standard, sizeof(int) * 100);
	printf("Address of ptr_standard: %p\n", ptr_standard);
	check_address(ptr_standard);
    free(ptr_standard);
    printf("Deallocation successful for ptr_standard\n\n");
	printf("Allocated blocks: %d\n", allocated_blocks);
	check_for_leaks();
	printf("---- Benchmark with custom _aligned_alloc ----\n");
	start = clock();
	int *ptr_aligned_custom = (int *)_aligned_alloc(32, sizeof(int) * num_elements);
	if (ptr_aligned_custom == NULL) {
		printf("Aligned allocation failed for ptr_aligned_custom\n");
		return 1;
	}
	for (size_t i = 0; i < num_elements; i++)
		ptr_aligned_custom[i] = i;
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Time taken with custom _aligned_alloc: %f seconds\n", cpu_time_used);
	_aligned_free(ptr_aligned_custom);
	printf("Deallocation successful for ptr_aligned_custom\n\n");
	


    printf("===== Memory Leak Check =====\n\n");
    printf("Number of blocks allocated: %d\n", allocated_blocks);
    if (allocated_blocks == 0)
        printf("No memory leaks detected\n");
    else
        printf("Memory leak detected\n");
	
	int *ptr2 = (int *)_malloc(sizeof(int) * 100);
	if (ptr2 == NULL) {
		printf("Allocation failed for ptr2\n");
		return 1;
	}
	printf("custom malloc\n");
	printf("Address of ptr2: %p\n", ptr2);
	check_address(ptr2);
	hexdump(ptr2, sizeof(int) * 100);
	_free(ptr2);

	int *ptr3 = (int *)malloc(sizeof(int) * 100);
	if (ptr3 == NULL) {
		printf("Allocation failed for ptr3\n");
		return 1;
	}
	printf("real malloc\n");
	printf("Address of ptr3: %p\n", ptr3);
	hexdump(ptr3, sizeof(int) * 100);
	free(ptr3);

	int *ptr4 = (int *)_malloc(sizeof(int) * 100);
	_free(ptr4);
	

	int *ptr5 = (int *)_malloc(sizeof(int) * rand());
	_free(ptr5);

	double *ptr7 = (double *)_malloc(sizeof(double) * 100);
	for (int i = 0; i < 100; i++)
		ptr7[i] = i;
	hexdump(ptr7, sizeof(double) * 100);
	check_address(ptr7);
	_free(ptr7);
	count_blocks(freelist);
	check_for_leaks();

	int *ptr6 = (int *)_malloc(sizeof(int) * 1000000);
	for (int i = 0; i < 1000000; i++)
		ptr6[i] = i;

	hexdump(ptr6, sizeof(int) * 100);
	_free(ptr6);
	count_blocks(freelist);
	check_for_leaks();

	char *str2 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";
	printf("String: %s\n", str2);
	char *dup_str2 = ft_strdup(str2);
	if (dup_str2 == NULL) {
		printf("String duplication failed\n");
		return 1;
	}
	printf("Duplicated string: %s\n", dup_str2);
	hexdump(dup_str, ft_strlen(dup_str2));
	check_address(dup_str2);
	_free(dup_str2);
	count_blocks(freelist);


	start = clock();
	int *ptr_custom2 = (int *)_malloc(sizeof(int) * num_elements);
	if (ptr_custom2 == NULL) {
		printf("Allocation failed for ptr_custom2\n");
		return 1;
	}
	for (size_t i = 0; i < num_elements; i++)
		ptr_custom2[i] = i;
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Time taken with custom _malloc: %f seconds\n", cpu_time_used);
	_free(ptr_custom2);
	count_blocks(freelist);
	check_for_leaks();

	start = clock();
	int *ptr_standard2 = (int *)malloc(sizeof(int) * num_elements);
	if (ptr_standard2 == NULL) {
		printf("Allocation failed for ptr_standard2\n");
		return 1;
	}
	for (size_t i = 0; i < num_elements; i++)
		ptr_standard2[i] = i;
	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("Time taken with standard malloc: %f seconds\n", cpu_time_used);
	free(ptr_standard2);
	count_blocks(freelist);
	check_for_leaks();


	printf("===== Running Modified Benchmark with Custom _malloc and Random Sizes =====\n\n");
	benchmark_malloc(1000000);

	//test itoa
	int num = 123456;	
	char *str_num = ft_itoa(num);
	printf("Number: %d\n", num);
	printf("String: %s\n", str_num);
	_free(str_num);

	test_random_alloc_free();
	test_alignment();
	test_large_allocations();
	test_small_allocations();
    return 0;
}
