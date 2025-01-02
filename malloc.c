#include "malloc.h"

static uint32_t __attribute__((__visibility__("hidden"))) bitmap[BITMAP_SIZE / 32] = {0}; 
static void *	__attribute__((__visibility__("hidden"))) memory_pool = NULL;
Block			__attribute__((__visibility__("hidden"))) *freelist = NULL;
static Block	__attribute__((__visibility__("hidden"))) *bins[NUM_BINS] = {NULL};

__attribute__((constructor))
static inline void initialize_memory_pool() {
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

static inline int get_bin_index(size_t size) {
    if (size <= 32) return size / BLOCK_UNIT_SIZE;
    if (size > 32 && size <= 512) return 32 + (size / 64);
    return 63; 
}

static inline void insert_bin(Block *block) {
    int index = get_bin_index(block->size);
    block->next = bins[index];
    bins[index] = block;
}

static inline Block *find_bin(size_t size) {
    int index = get_bin_index(size);
    for (; index < NUM_BINS; index++) {
        if (bins[index]) return bins[index];
    }
    return NULL; 
}

static inline void insert_sorted(Block *block) {
    Block *current = freelist;
    Block *prev = NULL;

    while (current && current->size < block->size) {
        prev = current;
        current = current->next;
    }

    block->next = current;
    if (prev) {
        prev->next = block;
    } else {
        freelist = block;
    }
}

static void *find_free_block(size_t size, size_t alignment) {
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
        if (bitmask) { 
            for (int j = 0; j < 8; j++) {
                size_t current_start = i * 8 + j;

                if (current_start == 0) 
					continue;

				if (current_start + units_needed > BITMAP_SIZE)
					continue;

                uint32_t bits = ~bitmap[current_start];
                while (bits) {
                    int free_bit = __builtin_ctz(bits);
                    size_t start = (current_start * 32) + free_bit;
                    uintptr_t addr = (uintptr_t)memory_pool + start * BLOCK_UNIT_SIZE;

                    if (addr + size > (uintptr_t)memory_pool + MEMORY_POOL_SIZE)
                        break;

                    int enough_space = 1;
                    for (size_t k = 0; k < units_needed; k++) {
                        size_t idx = start + k;
                        if (bitmap[idx / 32] & (1U << (idx % 32))) {
                            enough_space = 0;
                            break;
                        }
                    }

                    if (enough_space) {
                        for (size_t k = 0; k < units_needed; k++) {
                            size_t idx = start + k;
                            bitmap[idx / 32] |= (1U << (idx % 32));
                        }

                        uintptr_t aligned_addr = align_up(addr, alignment);
                        if (aligned_addr + size > (uintptr_t)memory_pool + MEMORY_POOL_SIZE) {
							LOG_ERROR("[DEBUG] Aligned address exceeds memory pool size.\n"); 
                            return NULL;
                        }
                        return (void *)aligned_addr;
                    }

                    bits &= ~(1U << free_bit);
                }
            }
        }
    }
    return NULL; 
}

static inline void split_block(Block *block, size_t size, size_t alignment) {
    size_t remaining_size = block->size - size - BLOCK_SIZE;
    if (remaining_size < BLOCK_SIZE) 
        return;

    uintptr_t new_block_address = (uintptr_t)block + BLOCK_SIZE + size;
    uintptr_t aligned_new_block_address = align_up(new_block_address, alignment);

    if ((aligned_new_block_address - new_block_address) > remaining_size) 
        return; 

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
		LOG_ERROR("[ERROR] Alignment must be a power of 2.\n"); 
        return NULL;
    }

    void *free_block = find_free_block(size, alignment);
    if (free_block) {
        uintptr_t block_addr = (uintptr_t)free_block - sizeof(Block);
        Block *block = (Block *)block_addr;

        if ((uintptr_t)block < (uintptr_t)memory_pool || 
            (uintptr_t)block + sizeof(Block) > (uintptr_t)memory_pool + MEMORY_POOL_SIZE) {
			LOG_ERROR("[ERROR] Invalid block address: %p\n", block); 
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
	madvise(request, total_size, MADV_HUGEPAGE); 
	if (request == MAP_FAILED) {
        perror("[ERROR] mmap failed");
        return NULL;
    }

    uintptr_t raw_addr = (uintptr_t)request;
    uintptr_t aligned_addr = align_up(raw_addr + sizeof(Block), alignment);

    if (aligned_addr + size > raw_addr + total_size) {
        if (munmap(request, total_size) == -1)
            perror("[ERROR] munmap failed");
		LOG_ERROR("[ERROR] Aligned address exceeds memory pool size.\n"); 
        return NULL;
    }

    Block *block = (Block *)(aligned_addr - sizeof(Block));

    if ((uintptr_t)block < raw_addr || 
        (uintptr_t)block + sizeof(Block) + size > raw_addr + total_size) {
        munmap(request, total_size);
		LOG_ERROR("[ERROR] Invalid block address: %p\n", block); 
        return NULL;
    }

    block->size = size;
    block->_free = 0;
    block->is_mmap = 1;
    block->next = NULL;
    block->aligned_address = (void *)__builtin_align_up(aligned_addr, alignment);

    if (last) 
        last->next = block;
    else if (!freelist)
        freelist = block;

    return block;
}


static inline void coalesce_free_blocks() {
    Block *current = freelist;

    while (current && current->next) {
        uintptr_t current_end = (uintptr_t)current + BLOCK_SIZE + current->size;
        uintptr_t next_start = (uintptr_t)current->next;

        if (current_end == next_start && current->_free && current->next->_free) {
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}


static inline void improved_free(void *ptr) {
    if (!ptr)
        return;

    if ((uintptr_t)ptr >= (uintptr_t)memory_pool && 
        (uintptr_t)ptr < (uintptr_t)memory_pool + MEMORY_POOL_SIZE) {
        
        Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
        if (block->_free)
            return; 

        
		block->_free = 1;
		insert_sorted(block);

    } else {
        Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
        if (block->is_mmap) {
            size_t total_size = block->size + sizeof(Block);
            if (munmap((void *)((uintptr_t)block), total_size) == -1)
                perror("munmap failed");
        }
    }
}

void *_malloc(size_t size) {
    if (size == 0) 
        return NULL;
    
	static int is_initialized = 0;
	if (!is_initialized) {
		initialize_memory_pool();
		is_initialized = 1;
	}

    size = align_up(size, ALIGNMENT);

    if (size > MMAP_THRESHOLD) {
        size_t total_size = size + BLOCK_SIZE;
        void *addr = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		madvise(addr, total_size, MADV_SEQUENTIAL);
        if (addr == MAP_FAILED) {
			perror("mmap failed");
            return NULL;
        }
        Block *block = (Block *)addr;
        block->size = size;
        block->_free = 0;
        block->is_mmap = 1;
        return (void *)(block + 1);
    }

    Block *block = request_space(freelist, size, ALIGNMENT);
    if (!block) 
        return NULL;

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
    if (!ptr) 
        return malloc(size); 
    
	if (size == 0) {
        free(ptr); 
        return NULL;
    }

    Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
    if (block->size >= size) 
        return ptr; 

    void *new_ptr = _malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        _free(ptr);
    }

    return new_ptr;
}


