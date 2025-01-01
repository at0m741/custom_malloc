#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <immintrin.h>
#include <sys/mman.h>
#include <unistd.h>

#include <time.h>
#define NUM_SMALL_ALLOCS 1000
#define NUM_LARGE_ALLOCS 100
#define MAX_ALLOCATION_SIZE (1 << 20)
#define BLOCK_UNIT_SIZE 1024
#define MEMORY_POOL_SIZE (1024 * 1024)
#define BITMAP_SIZE (MEMORY_POOL_SIZE / BLOCK_UNIT_SIZE)
#define ALIGNMENT 32
#define BLOCK_SIZE sizeof(Block)



static uint32_t bitmap[BITMAP_SIZE / 32] = {0}; 

static void *memory_pool = NULL;

typedef struct __attribute__((aligned(ALIGNMENT))) Block {
    size_t size;
    int _free;
    int is_mmap;
    struct Block *next;
    void *aligned_address;
} Block;

Block *freelist = NULL;

static inline uintptr_t align_up(uintptr_t addr, size_t alignment) {
    return (addr + (alignment - 1)) & ~(alignment - 1);
}


__attribute__((constructor))
static void initialize_memory_pool() {
    memory_pool = mmap(NULL, MEMORY_POOL_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_pool == MAP_FAILED) {
        perror("Failed to initialize memory pool");
        exit(EXIT_FAILURE);
    }

    freelist = (Block *)memory_pool;
    freelist->size = MEMORY_POOL_SIZE - BLOCK_SIZE;
    freelist->_free = 1;
    freelist->is_mmap = 0;
    freelist->next = NULL;
    freelist->aligned_address = (void *)((uintptr_t)memory_pool + BLOCK_SIZE);
}


__attribute__((hot))
void *find_free_block(size_t size, size_t alignment) {
    if (!memory_pool || size == 0 || size > MEMORY_POOL_SIZE)
        return NULL;

    size_t units_needed = (size + BLOCK_UNIT_SIZE - 1) / BLOCK_UNIT_SIZE;
    __m256i mask = _mm256_set1_epi32(0xFFFFFFFF);

    for (size_t i = 0; i < BITMAP_SIZE / 256; i++) {
		_mm_prefetch((const char *)&bitmap[i * 8], _MM_HINT_T0);
		_mm_prefetch((const char *)&bitmap[(i + 1) * 8], _MM_HINT_T0);

        __m256i bitmap_chunk = _mm256_loadu_si256((__m256i *)&bitmap[i * 8]);
        __m256i inverted_chunk = _mm256_andnot_si256(bitmap_chunk, mask);
        int bitmask = _mm256_movemask_epi8(inverted_chunk);

        if (__builtin_expect(bitmask != 0, 0)) {
            for (int j = 0; j < 8; j++) {
                size_t current_start = i * 8 + j;

                if (current_start == 0)
                    continue;

                uint32_t bits = ~bitmap[current_start];
                if (__builtin_expect(bits != 0, 0)) {
                    int free_bit = __builtin_ctz(bits);
                    size_t start = (current_start * 32) + free_bit;
                    uintptr_t addr = (uintptr_t)memory_pool + start * BLOCK_UNIT_SIZE;

                    if (addr + size > (uintptr_t)memory_pool + MEMORY_POOL_SIZE)
                        return NULL;

                    int enough_space = 1;
                    for (size_t k = 0; k < units_needed; k++) {
                        size_t idx = start + k;
                        if (bitmap[idx / 32] & (1U << (idx % 32))) {
                            enough_space = 0;
                            break;
                        }
                    }

                    if (__builtin_expect(enough_space, 0)) {
                        for (size_t k = 0; k < units_needed; k++) {
                            size_t idx = start + k;
                            if (idx / 32 >= BITMAP_SIZE / 32) {
                                fprintf(stderr, "Erreur : Dépassement de la taille du bitmap.\n");
                                return NULL;
                            }
                            bitmap[idx / 32] |= (1U << (idx % 32));
                        }
                    
                        uintptr_t aligned_addr = align_up(addr, alignment);
                        if (aligned_addr + size > (uintptr_t)memory_pool + MEMORY_POOL_SIZE) {
                            fprintf(stderr, "Erreur : Adresse alignée en dehors des limites.\n");
                            return NULL;
                        }
                        return (void *)aligned_addr;
                    }
                }
            }
        }
    }
    return NULL;
}


static inline void split_block(Block *block, size_t size, size_t alignment) {
    size_t remaining_size = block->size - size - BLOCK_SIZE;
    if (remaining_size < BLOCK_SIZE) {
        return;
    }

    uintptr_t new_block_address = (uintptr_t)block + BLOCK_SIZE + size;
    uintptr_t aligned_new_block_address = align_up(new_block_address, alignment);

    if ((aligned_new_block_address - new_block_address) > remaining_size) {
        return; 
    }

    Block *new_block = (Block *)aligned_new_block_address;
    new_block->size = remaining_size - (aligned_new_block_address - new_block_address);
    new_block->_free = 1;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;

}


Block *request_space(Block *last, size_t size, size_t alignment) {
    if (__builtin_expect(size == 0, 0)) 
        return NULL;

    if ((alignment & (alignment - 1)) != 0) {
        fprintf(stderr, "[ERROR] Alignment must be a power of two.\n");
        return NULL;
    }

    void *free_block = find_free_block(size, alignment);
    if (free_block) {
        uintptr_t block_addr = (uintptr_t)free_block - sizeof(Block);
        Block *block = (Block *)block_addr;

        if ((uintptr_t)block < (uintptr_t)memory_pool || 
            (uintptr_t)block + sizeof(Block) > (uintptr_t)memory_pool + MEMORY_POOL_SIZE) {
            fprintf(stderr, "[ERROR] pool Invalid block address: %p\n", block);
			return NULL;
        }
		if (size == 0) {
			fprintf(stderr, "[ERROR] Invalid block size: %zu\n", size);
			return NULL;
		}
        if (block->size > size + sizeof(Block)) {
            split_block(block, size, alignment);
        }

        block->size = size;
        block->_free = 0;
        block->aligned_address = free_block;
        return block;
    }

    size_t alignment_mask = alignment - 1;
    size_t total_size = size + sizeof(Block) + alignment_mask;
    size_t page_size = sysconf(_SC_PAGESIZE);
    total_size = (total_size + page_size - 1) & ~(page_size - 1);

    void *request = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (request == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }

    uintptr_t raw_addr = (uintptr_t)request;
    uintptr_t aligned_addr = align_up(raw_addr + sizeof(Block), alignment);

    if (aligned_addr + size > raw_addr + total_size) {
        if (munmap(request, total_size) == -1)
            perror("munmap failed");
        fprintf(stderr, "[Error] Aligned address exceeds allocated memory.\n");
        return NULL;
    }

    Block *block = (Block *)(aligned_addr - sizeof(Block));
    if ((uintptr_t)block < raw_addr || 
        (uintptr_t)block + sizeof(Block) + size > raw_addr + total_size) {
        munmap(request, total_size);
        fprintf(stderr, "[Error] Invalid block address: %p\n", block);
		printf("raw_addr: %ld\n", raw_addr);
        return NULL;
    }

    block->size = size;
    block->_free = 0;
    block->is_mmap = 1;
    block->next = NULL;
    block->aligned_address = (void *)aligned_addr;

    if (last)
        last->next = block;
    else if (!freelist)
        freelist = block;
    return block;
}


static inline void coalesce_free_blocks() {
    Block *current = freelist;
    while (current && current->next) {
        if (current->_free && current->next->_free) {
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

static inline void improved_free(void *ptr) {
    if (!ptr) {
        return;
    }

    if ((uintptr_t)ptr >= (uintptr_t)memory_pool && 
        (uintptr_t)ptr < (uintptr_t)memory_pool + MEMORY_POOL_SIZE) {
        
        Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
        if (block->_free) {
            return; 
        }

        block->_free = 1;


    } else {
        Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
        if (block->is_mmap) {
            size_t total_size = block->size + sizeof(Block);
            if (munmap((void *)((uintptr_t)block), total_size) == -1) {
                perror("munmap failed");
            }
        }
    }
}


void *_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
	initialize_memory_pool();
    size = align_up(size, ALIGNMENT);
    Block *block = request_space(freelist, size, ALIGNMENT);
    if (!block) {
        fprintf(stderr, "[ERROR] Failed to allocate memory.\n");
        return NULL;
    }

    if ((uintptr_t)block->aligned_address < (uintptr_t)memory_pool ||
        (uintptr_t)block->aligned_address + size > (uintptr_t)memory_pool + MEMORY_POOL_SIZE) {
		return NULL;
    }

    return block->aligned_address;
}

void _free(void *ptr) {
	improved_free(ptr);
	coalesce_free_blocks();
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return _malloc(size); 
    }
    if (size == 0) {
        _free(ptr); 
        return NULL;
    }

    Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
    if (block->size >= size) {
        return ptr; 
    }

    void *new_ptr = _malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        _free(ptr);
    }
    return new_ptr;
}



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

int main() {
    printf("Test simplifié de _malloc et _free:\n\n");

    void *ptr1 = _malloc(128);
    void *ptr2 = _malloc(256);
    printf("[TEST] ptr1: %p, ptr2: %p\n", ptr1, ptr2);

    if (ptr1) {
        printf("[TEST] Freeing block 1.\n");
        _free(ptr1);
    }
    if (ptr2) {
        printf("[TEST] Freeing block 2.\n");
        _free(ptr2);
    }

    printf("\nTest de plusieurs allocations:\n");
    void *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = _malloc(64 * (i + 1));
        printf("[TEST] Allocating block %d at %p\n", i + 1, ptrs[i]);    
	}

    for (int i = 0; i < 10; i++) {
        if (ptrs[i]) {
            printf("Libération du bloc %d.\n", i + 1);
            _free(ptrs[i]);
        }
    }

    printf("\nAlignments tests:\n");
    for (int i = 0; i < 5; i++) {
        void *aligned_ptr = aligned_alloc(32, 128 * (i + 1));
        if (aligned_ptr) {
            printf("Bloc aligné alloué à : %p\n", aligned_ptr);
            free(aligned_ptr);
        } else {
            printf("Erreur lors de l'allocation alignée.\n");
        }
    }

    printf("\nWrite test:\n");
    void *ptr3 = _malloc(512);
    if (ptr3) {
        memset(ptr3, 0xAA, 512);
        printf("Bloc alloué et rempli à : %p\n", ptr3);
        _free(ptr3);
    }


	printf("\nRealloc test:\n");
	void *ptr4 = _malloc(128);
	if (ptr4) {
		printf("Bloc alloué à : %p\n", ptr4);
		ptr4 = my_realloc(ptr4, 256);
		if (ptr4) {
			printf("Bloc réalloué à : %p\n", ptr4);
			_free(ptr4);
		}
	}


	
	/* test_random_alloc_free(); */

	printf("\nAlloc Null test:\n");
	void *ptr5 = _malloc(0);
	if (ptr5) {
		printf("Bloc alloué à : %p\n", ptr5);
		_free(ptr5);
	}
	printf("\nAlloc Null test:\n");
	ptr5 = _malloc(0);
	if (ptr5) {
		printf("Bloc alloué à : %p\n", ptr5);
		_free(ptr5);
	}
	printf("\n[PASS] Alloc Null test\n");
	
	benchmark_malloc(100000);

    printf("\nTous les tests sont terminés avec succès.\n");
    return 0;
}
