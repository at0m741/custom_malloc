#include "include.h"

Block *freelist = NULL;
int allocated_blocks = 0;
int freed_blocks = 0;
Block *bins[BIN_COUNT] = {NULL};

/*
	* this is the custom malloc function
	* size: size of the memory to be allocated
	* Returns: pointer to the allocated memory
*/	

void *_malloc(size_t size) {
    if (__builtin_expect(size == 0, 0))
        return NULL;
    size = ALIGN(size, ALIGNMENT);
    if (size <= BIN_MAX_SIZE) 
	{
        int bin_index = size / ALIGNMENT - 1;
        if (bin_index >= 0 && bin_index < BIN_COUNT && bins[bin_index]) 
		{
            #if DEBUG
				printf("Allocating from bin %d\n", bin_index);
				printf("Block size: %zu\n", size);
				printf("\n");
            #endif
            Block *block = bins[bin_index];
            bins[bin_index] = block->next;
            block->free = 0;
            return block->aligned_address;
        }
    }

    Block *block;
    if (size >= MMAP_THRESHOLD)
        return request_space_mmap(size, ALIGNMENT);
	else
	{
        if (!freelist) 
		{
            block = request_space_sbrk(NULL, size, ALIGNMENT);
            if (__builtin_expect(!block, 0))
                return NULL;
            freelist = block;
        } 
		else 
		{
            Block *last = freelist;
            block = find_free_block(&last, size, ALIGNMENT);
            if (block) 
			{
                if (block->size >= size + BLOCK_SIZE)
				{
					printf("HHHHHHHHHHHHHHHH\n");
                    split_block(block, size, ALIGNMENT);
				}
				block->free = 0;
            } 
			else 
			{
                block = request_space_sbrk(last, size, ALIGNMENT);
                if (__builtin_expect(!block, 0))
                    return NULL;
            }
        }
    }
    return block->aligned_address;
}


/*
	* this is the custom aligned malloc function 
	* alignment: alignment of the memory to be allocated
	* size: size of the memory to be allocated
	* Returns: pointer to the allocated memory
	*
	* This function is used to allocate memory with a specific alignment
	* principaly used for SIMD instructions (SSE, AVX, etc.)
	* The function will allocate a block of memory with a size greater than the requested size
	* The function will then align the memory to the requested alignment
	* The function will then return the aligned memory
	* The function will use sbrk to allocate memory if the requested size is larger than the block size
	* The function will use mmap to allocate memory if the requested size is larger than the mmap threshold
	*
*/


void *_aligned_alloc(size_t alignment, size_t size) {
    if (__builtin_expect(size == 0, 0) || (alignment & (alignment - 1)) != 0)
        return NULL;
    
    size = ALIGN(size, alignment);
    Block *block;
    
    if (size <= CACHE_SIZE_L1) {
        _mm_prefetch(freelist, _MM_HINT_T0);
    } else if (size <= CACHE_SIZE_L2) {
        _mm_prefetch(freelist, _MM_HINT_T1);
    }

    if (size >= MMAP_THRESHOLD) {
        return request_space_mmap(size, alignment);
    } else {
        if (!freelist) {
            block = request_space_sbrk(NULL, size, alignment);
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
            } else {
                block = request_space_sbrk(last, size, alignment);
                if (__builtin_expect(!block, 0))
                    return NULL;
            }
        }
    }

    uintptr_t addr = (uintptr_t)block->aligned_address;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    block->aligned_address = (void *)aligned_addr;

    if (alignment >= 32 && __builtin_cpu_supports("avx")) {
        _memset_avx(block->aligned_address, 0, size);
    }

    return block->aligned_address;
}
