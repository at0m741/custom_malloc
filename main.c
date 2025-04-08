#include "malloc.h"

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

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
                fprintf(stderr, RED"[ERROR] Error: Random allocation failed\n"RESET);
                exit(EXIT_FAILURE);
            }
        }
    }

    for (size_t i = 0; i < NUM_SMALL_ALLOCS; i++) {
        if (allocations[i]) {
            _free(allocations[i]);
        }
    }


    printf(GREEN"[TEST/OK] Random alloc/free test passed.\n"RESET);
}

void check_alignment(void *ptr) {
	if ((uintptr_t)ptr % 16 != 0)
		printf(RED"[ALIGN] Memory not aligned to 16 bytes\n"RESET);
	else
		printf(GREEN"[ALIGN] Memory aligned to 16 bytes\n"RESET);
}


void test_posix_memalign_custom() {
    printf("\n== _posix_memalign Tests ==\n");

    void *ptr;
    int ret;

    ret = _posix_memalign(&ptr, 20, 512);
    if (ret != 0) {
        printf(GREEN"[TEST] _posix_memalign correctly returned error for invalid alignment (20). Error code: %d\n"RESET, ret);
    } else {
        printf(RED"[ERROR] _posix_memalign did not return an error for invalid alignment (20).\n"RESET);
    }

    ret = _posix_memalign(&ptr, 16, 512);
    if (ret == 0 && ptr != NULL && ((uintptr_t)ptr % 16 == 0)) {
        printf(GREEN"[TEST] _posix_memalign succeeded for alignment 16. Pointer: %p\n"RESET, ptr);
        _free(ptr);
    } else {
        printf(RED"[ERROR] _posix_memalign failed for alignment 16. Error code: %d, Pointer: %p\n"RESET, ret, ptr);
    }

    ret = _posix_memalign(&ptr, 32, 1024);
    if (ret == 0 && ptr != NULL && ((uintptr_t)ptr % 32 == 0)) {
        printf(GREEN"[TEST] _posix_memalign succeeded for alignment 32. Pointer: %p\n"RESET, ptr);
        _free(ptr);
    } else {
        printf(RED"[ERROR] _posix_memalign failed for alignment 32. Error code: %d, Pointer: %p\n"RESET, ret, ptr);
    }

    ret = _posix_memalign(&ptr, 64, 2048);
    if (ret == 0 && ptr != NULL && ((uintptr_t)ptr % 64 == 0)) {
        printf(GREEN"[TEST] _posix_memalign succeeded for alignment 64. Pointer: %p\n"RESET, ptr);
        _free(ptr);
    } else {
        printf(RED"[ERROR] _posix_memalign failed for alignment 64. Error code: %d, Pointer: %p\n"RESET, ret, ptr);
    }

    ret = _posix_memalign(&ptr, 256, 4096);
    if (ret == 0 && ptr != NULL && ((uintptr_t)ptr % 256 == 0)) {
        printf(GREEN"[TEST] _posix_memalign succeeded for alignment 256. Pointer: %p\n"RESET, ptr);
        _free(ptr);
    } else {
        printf(RED"[ERROR] _posix_memalign failed for alignment 256. Error code: %d, Pointer: %p\n"RESET, ret, ptr);
    }

    printf(GREEN"[TEST/OK] _posix_memalign tests completed.\n"RESET);
}
void benchmark_malloc(size_t num_elements) {
    struct timespec start, end;
    double time_taken;
	printf("\n\n== Benchmark Test ==\n");
    printf("---- Benchmark with custom _malloc ----\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    int *ptr_custom = (int *)_malloc(sizeof(int) * num_elements);
    if (ptr_custom == NULL) {
        printf(GREEN"[TEST/OK] Allocation failed for ptr_custom\n"RESET);
        return;
    }
    for (size_t i = 0; i < num_elements; i++)
        ptr_custom[i] = i;
    clock_gettime(CLOCK_MONOTONIC, &end);

    time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf(GREEN"[BENCH] Time taken with custom _malloc: %f seconds\n"RESET, time_taken);
    _free(ptr_custom);
	
    printf("---- Benchmark with standard malloc ----\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    int *ptr_standard = (int *)malloc(sizeof(int) * num_elements);
	printf(YELLOW"[DEBUG] ptr_standard: %p\n"RESET, ptr_standard);
    if (ptr_standard == NULL) {
        printf(RED"[ERROR] Allocation failed for ptr_standard\n"RESET);
        return;
    }
    for (size_t i = 0; i < num_elements; i++)
        ptr_standard[i] = i;
    clock_gettime(CLOCK_MONOTONIC, &end);

    time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf(GREEN"[BENCH] Time taken with standard malloc: %f seconds\n"RESET, time_taken);
    free(ptr_standard);
	check_alignment(ptr_standard);

	void *aligned_ptr;
	clock_gettime(CLOCK_MONOTONIC, &start);
	if (posix_memalign(&aligned_ptr, 16, sizeof(int) * num_elements) == 0) {
		for (size_t i = 0; i < num_elements; i++)
			((int *)aligned_ptr)[i] = i;
		clock_gettime(CLOCK_MONOTONIC, &end);
		time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
		printf(GREEN"[BENCH] Time taken with posix_memalign: %f seconds\n"RESET, time_taken);
		free(aligned_ptr);
	} else {
		printf(RED"[ERROR] Allocation failed for aligned_ptr\n"RESET);
	}

	void *aligned_ptr2 __attribute__((aligned(32)));
	clock_gettime(CLOCK_MONOTONIC, &start);
	if (_posix_memalign(&aligned_ptr2, 32, sizeof(int) * num_elements) == 0) {
		for (size_t i = 0; i < num_elements; i++)
			((int *)aligned_ptr2)[i] = i;
		clock_gettime(CLOCK_MONOTONIC, &end);
		time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
		printf(GREEN"[BENCH] Time taken with _posix_memalign: %f seconds\n"RESET, time_taken);
		_free(aligned_ptr2);
	} else {
		printf(RED"[ERROR] Allocation failed for aligned_ptr2\n"RESET);
	}

	__m256i vec = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
	void *vec_ptr;

	clock_gettime(CLOCK_MONOTONIC, &start);
	if (_posix_memalign(&vec_ptr, 32, sizeof(int) * num_elements) == 0) {
		for (size_t i = 0; i < num_elements; i += 8)
			_mm256_store_si256((__m256i *)((int *)vec_ptr + i), vec);
		clock_gettime(CLOCK_MONOTONIC, &end);
		time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
		printf(GREEN"[BENCH] Time taken with posix_memalign + AVX store: %f seconds\n"RESET, time_taken);
		_free(vec_ptr);
	} else {
		printf(RED"[ERROR] Allocation failed for vec_ptr\n"RESET);
	}

	printf("\n");
	printf("=====================================\n");
	printf("=                                   =\n");
	printf("=         Benchmark Completed       =\n");
	printf("=                                   =\n");
	printf("=====================================\n");
	printf("\n");

	printf(GREEN"[TEST/OK] Benchmark tests completed.\n"RESET);
}

void benchmark_memcpy_methods(size_t num_bytes) {
    struct timespec start, end;
    double elapsed;

    // Allocation alignée
    void *src, *dst;
    _posix_memalign(&src, 32, num_bytes);
    _posix_memalign(&dst, 32, num_bytes);

    // Init de src
    memset(src, 42, num_bytes);

    printf("\n== memcpy/memmove/AVX benchmark (%lu bytes) ==\n", num_bytes);

    // --- memcpy ---
    clock_gettime(CLOCK_MONOTONIC, &start);
    memcpy(dst, src, num_bytes);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf(GREEN"[BENCH] memcpy        : %f sec\n"RESET, elapsed);

    // --- memmove ---
    clock_gettime(CLOCK_MONOTONIC, &start);
    memmove(dst, src, num_bytes);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf(GREEN"[BENCH] memmove       : %f sec\n"RESET, elapsed);

    // --- AVX2 memcpy ---
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < num_bytes; i += 32) {
        __m256i data = _mm256_load_si256((__m256i *)((char *)src + i));
		_mm256_stream_si256((__m256i *)((char *)dst + i), data);

    }
	_mm_sfence();
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf(GREEN"[BENCH] AVX2 memcpy   : %f sec\n"RESET, elapsed);

    _free(src);
    _free(dst);
}


void test_large_allocations() {
    printf("\n== Large Allocations Test ==\n");
    void *large_ptr1 = _malloc(1024 * 1024); // 1 MB
    void *large_ptr2 = _malloc(2 * 1024 * 1024); // 2 MB

    if (large_ptr1 && large_ptr2) {
        printf(GREEN"[TEST] Large blocks allocated successfully.\n"RESET);
    } else {
        printf(RED"[ERROR] Large allocation failed.\n"RESET);
    }

    _free(large_ptr1);
    _free(large_ptr2);

    printf(GREEN"[TEST/OK] Large allocations test passed.\n"RESET);
}

void test_heap_overflow() {
    char *ptr = (char *)_malloc(10);
    strcpy(ptr, "This string is too long for the allocated space"); 
	printf(GREEN"[SEC] Overflow: %s\n"RESET, ptr);
	printf(GREEN"[SEC] ptr: %p\n"RESET, ptr);
	printf(GREEN"[SEC] ptr + 10: %p\n"RESET, ptr + 10);
}

char *ft_strdup(const char *s) {
	size_t len = strlen(s);
	char *new_s = (char *)_malloc(len + 1);
	if (new_s == NULL) {
		return NULL;
	}
	strcpy(new_s, s);
	return new_s;
}

void test_heap_underflow() {
    char *ptr = (char *)_malloc(10);
    char *under_ptr = ptr - 5;
    strcpy(under_ptr, "Underflow");
	printf(GREEN"[SEC] Underflow: %s\n"RESET, under_ptr);
	printf(GREEN"[SEC] ptr: %p\n"RESET, ptr);
	printf(GREEN"[SEC] under_ptr: %p\n"RESET, under_ptr);
}

void test_edge_case_allocations() {
    printf("\n== Edge Case Allocations Test ==\n");

    void *ptr1 = _malloc(1); 
    void *ptr2 = _malloc(MEMORY_POOL_SIZE); 
    void *ptr3 = _malloc(MEMORY_POOL_SIZE + MEMORY_POOL_SIZE / 2 + 1);
	void *ptr4 = _malloc(MEMORY_POOL_SIZE / 2);

    if (ptr1) {
        printf(GREEN"[TEST] Smallest allocation succeeded.\n"RESET);
        _free(ptr1);
    } else {
        printf(RED"[ERROR] Smallest allocation failed.\n"RESET);
    }

    if (ptr2) {
        printf(GREEN"[TEST] Pool-size allocation succeeded.\n"RESET);
        _free(ptr2);
    } else {
        printf(RED"[ERROR] Pool-size allocation failed.\n"RESET);
    }

    if (!ptr3) {
        printf(GREEN"[TEST] Oversized allocation correctly failed.\n"RESET);
    } else {
        printf(RED"[ERROR] Oversized allocation unexpectedly succeeded.\n"RESET);
        _free(ptr3);
    }

	if (!ptr4) {
		printf(GREEN"[TEST] Oversized allocation correctly failed.\n"RESET);
	} else {
		printf(RED"[ERROR] Oversized allocation unexpectedly succeeded.\n"RESET);
		_free(ptr4);
	}
	
    printf(GREEN"[TEST/OK] Edge case allocations test passed.\n"RESET);
}

void test_repeated_alloc_free() {
    printf("\n== Repeated Alloc/Free Test ==\n");

    for (size_t i = 0; i < 1000; i++) {
        void *ptr = _malloc(1024); 
        if (!ptr) {
            printf(RED"[ERROR] Allocation failed in iteration %zu.\n"RESET, i);
            exit(EXIT_FAILURE);
        }
        _free(ptr);
    }

    printf(GREEN"[TEST/OK] Repeated alloc/free test passed.\n"RESET);
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
        printf(GREEN"[TEST] Large block allocated after fragmentation.\n"RESET);
        _free(large_ptr);
    } else {
        printf(RED"[ERROR] Large allocation failed due to fragmentation.\n"RESET);
		exit(EXIT_FAILURE);
    }
	//posix_memalign
	__attribute__((aligned(32))) void *aligned_ptr;
	if (_posix_memalign(&aligned_ptr, 32, 512) == 0) {
		printf(GREEN"[TEST] Aligned allocation succeeded.\n"RESET);
		_free(aligned_ptr);
	} else {
		printf(RED"[ERROR] Aligned allocation failed.\n"RESET);
	}

    for (int i = 1; i < 10; i += 2) {
        _free(ptrs[i]);
    }

    printf(GREEN"[TEST/OK] Fragmentation test passed.\n"RESET);
}

void test_alignment() {
    printf("\n== Alignment Test ==\n");

    for (size_t alignment = 16; alignment <= 256; alignment *= 2) {
        void *ptr = aligned_alloc(alignment, 512);
        if (ptr) {
            if ((uintptr_t)ptr % alignment == 0) {
                printf(GREEN"[TEST] Alignment %zu bytes succeeded.\n"RESET, alignment);
            } else {
                printf(RED"[ERROR] Alignment %zu bytes failed.\n"RESET, alignment);
            }
            free(ptr);
        } else {
            printf(RED"[ERROR] Aligned allocation failed for alignment %zu.\n"RESET, alignment);
        }
    }

    printf(GREEN"[TEST/OK] Alignment test passed.\n"RESET);
}

int main() {
    printf(" =====================================\n");
	printf("=				      =\n");
	printf("= Custom Malloc Implementation Tests  =\n");
	printf("=				      =\n");
	printf(" =====================================\n");

    test_random_alloc_free();
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
    benchmark_malloc(3000000);
	printf("\n== Heap Overflow/Underflow Tests ==\n");
	test_heap_overflow();
	test_heap_underflow();
	
	printf("\n== strdup Tests ==\n");

	printf(GREEN"[DUP] ft_strdup: %s\n"RESET, ft_strdup("Hello, world!"));
	printf(GREEN"[DUP] ft_strdup: %s\n"RESET, ft_strdup("This is a test"));
	printf(GREEN"[DUP] ft_strdup: %s\n"RESET, ft_strdup("Another test"));
	printf(GREEN"[DUP] ft_strdup: %s\n"RESET, ft_strdup("Yet another test"));
	printf(GREEN"[DUP] ft_strdup: %s\n"RESET, ft_strdup("Final test"));

	test_posix_memalign_custom();
	benchmark_memcpy_methods(64 * 1024 * 1024); // 1 MB
    printf(GREEN"\n[OK] All tests completed successfully.\n"RESET);
    return 0;
}
