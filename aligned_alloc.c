#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;



void *_aligned_alloc(size_t alignment, size_t size) {
    if (__builtin_expect(size == 0, 0) || (alignment & (alignment - 1)) != 0)
        return NULL;
    
    size = ALIGN(size, alignment);
    Block *block = NULL;
    
    if (size >= MMAP_THRESHOLD) {
        return request_space_mmap(size, alignment);
    } else {
        if (!freelist) {
            block = request_space(NULL, size, alignment);
            if (__builtin_expect(!block, 0))
                return NULL;
            freelist = block;
        } else {
            Block *last = freelist;
            block = find_free_block(&last, size, alignment);
            if (block) {
                if (block->size >= size + BLOCK_SIZE)
                    split_block(block, size, alignment);
                block->free = 0;
                memset(block->aligned_address, 0, size); 
            } else {
                block = request_space(last, size, alignment);
                if (__builtin_expect(!block, 0))
                    return NULL;
            }
        }
    }

    uintptr_t addr = (uintptr_t)block->aligned_address;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    block->aligned_address = (void *)aligned_addr;

	_memset_avx(block->aligned_address, 0, size);

    return block->aligned_address;
}
