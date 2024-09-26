#include "include.h"

Block *freelist = NULL;
int allocated_blocks = 0;
int freed_blocks = 0;
Block *bins[BIN_COUNT] = {NULL};
Block *is_mmap = NULL;

/*
	* this is the custom malloc function
	* size: size of the memory to be allocated
	* Returns: pointer to the allocated memory
*/	


void *_malloc(size_t size) {
    if (__builtin_expect(size == 0, 0))
        return NULL;
    size = ALIGN(size, ALIGNMENT);

    Block *block = NULL;
    if (size <= BIN_MAX_SIZE) {
        int bin_index = size / ALIGNMENT - 1;
        if (bin_index >= 0 && bin_index < BIN_COUNT && bins[bin_index]) {
            #if DEBUG
                printf("Allocating from bin %d\n", bin_index);
                printf("Block size: %zu\n", size);
                printf("\n");
            #endif
            Block *block = bins[bin_index];
            bins[bin_index] = block->next;
            block->free = 0;
            _memset_avx(block->aligned_address, 0, size);
            return block->aligned_address;
        }
    }

    if (size >= MMAP_THRESHOLD)
        return request_space_mmap(size, ALIGNMENT);
    else {
        if (!freelist) {
            block = request_space_sbrk(NULL, size, ALIGNMENT);
            if (__builtin_expect(!block, 0))
                return NULL;
            freelist = block;
        } else {
            Block *last = freelist;
            block = find_free_block(&last, size, ALIGNMENT);
            if (block) {
                if (block->size >= size + BLOCK_SIZE)
                    split_block(block, size, ALIGNMENT);
                block->free = 0;
                _memset_avx(block->aligned_address, 0, size);
            } else {
                block = request_space_sbrk(last, size, ALIGNMENT);
                if (__builtin_expect(!block, 0))
                    return NULL;
            }
        }
    }

    return block->aligned_address;
}

