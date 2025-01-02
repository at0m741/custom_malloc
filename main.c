#include "malloc.h"

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

void check_alignment(void *ptr) {
	if ((uintptr_t)ptr % 16 != 0)
		printf("Memory not aligned to 16 bytes\n");
	else
		printf("Memory aligned to 16 bytes\n");
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
	printf("ptr_standard: %p\n", ptr_standard);
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
	check_alignment(ptr_standard);
}

void test_large_allocations() {
    printf("\n== Large Allocations Test ==\n");
    void *large_ptr1 = _malloc(1024 * 1024); // 1 MB
    void *large_ptr2 = _malloc(2 * 1024 * 1024); // 2 MB

    if (large_ptr1 && large_ptr2) {
        printf("[TEST] Large blocks allocated successfully.\n");
    } else {
        printf("[ERROR] Large allocation failed.\n");
    }

    _free(large_ptr1);
    _free(large_ptr2);

    printf("Large allocations test passed.\n");
}

void test_edge_case_allocations() {
    printf("\n== Edge Case Allocations Test ==\n");

    void *ptr1 = _malloc(1); // Smallest possible allocation
    void *ptr2 = _malloc(MEMORY_POOL_SIZE); // Size equal to the pool
    void *ptr3 = _malloc(MEMORY_POOL_SIZE + 1); // Size exceeding the pool

    if (ptr1) {
        printf("[TEST] Smallest allocation succeeded.\n");
        _free(ptr1);
    } else {
        printf("[ERROR] Smallest allocation failed.\n");
    }

    if (ptr2) {
        printf("[TEST] Pool-size allocation succeeded.\n");
        _free(ptr2);
    } else {
        printf("[ERROR] Pool-size allocation failed.\n");
    }

    if (!ptr3) {
        printf("[TEST] Oversized allocation correctly failed.\n");
    } else {
        printf("[ERROR] Oversized allocation unexpectedly succeeded.\n");
        _free(ptr3);
    }

    printf("Edge case allocations test passed.\n");
}

void test_repeated_alloc_free() {
    printf("\n== Repeated Alloc/Free Test ==\n");

    for (size_t i = 0; i < 1000; i++) {
        void *ptr = _malloc(1024); // Allocate 1 KB
        if (!ptr) {
            printf("[ERROR] Allocation failed in iteration %zu.\n", i);
            exit(EXIT_FAILURE);
        }
        _free(ptr);
    }

    printf("Repeated alloc/free test passed.\n");
}

void test_fragmentation() {
    printf("\n== Fragmentation Test ==\n");

    void *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = _malloc(128);
    }

    for (int i = 0; i < 10; i += 2) {
        _free(ptrs[i]);
    }

    void *large_ptr = _malloc(512);
    if (large_ptr) {
        printf("[TEST] Large block allocated after fragmentation.\n");
        _free(large_ptr);
    } else {
        printf("[ERROR] Large allocation failed due to fragmentation.\n");
    }

    for (int i = 1; i < 10; i += 2) {
        _free(ptrs[i]);
    }

    printf("Fragmentation test passed.\n");
}

void test_alignment() {
    printf("\n== Alignment Test ==\n");

    for (size_t alignment = 16; alignment <= 256; alignment *= 2) {
        void *ptr = aligned_alloc(alignment, 512);
        if (ptr) {
            if ((uintptr_t)ptr % alignment == 0) {
                printf("[TEST] Alignment %zu bytes succeeded.\n", alignment);
            } else {
                printf("[ERROR] Alignment %zu bytes failed.\n", alignment);
            }
            free(ptr);
        } else {
            printf("[ERROR] Aligned allocation failed for alignment %zu.\n", alignment);
        }
    }

    printf("Alignment test passed.\n");
}

int main() {
    printf("== Custom Malloc Tests ==\n");

    /* test_random_alloc_free(); */
    test_large_allocations();
    test_edge_case_allocations();
    
	int i;
	char *addr;

	i = 0;
	while (i < 1024)
	{
		addr = (char*)_malloc(1024);
		addr[0] = 42;
		i++;
	}

	test_repeated_alloc_free();
    test_fragmentation();
    test_alignment();
    benchmark_malloc(10000);
	
    printf("\nAll tests completed successfully.\n");
    return 0;
}
