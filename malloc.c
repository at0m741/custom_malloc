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
	{
		#ifdef DEBUG
			printf("Invalid size, by passing %zu\n", size);
		#endif
        return NULL;
	}
    size = ALIGN(size, ALIGNMENT);

    if (size <= BIN_MAX_SIZE) 
	{
        int bin_index = size / ALIGNMENT - 1;
        if (bin_index >= 0 && bin_index < BIN_COUNT && bins[bin_index]) 
		{
            #if DEBUG
				printf("Allocating from bin %d\n", bin_index);
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
                    split_block(block, size);
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


void *_aligned_alloc(size_t alignment, size_t size) {
    if (__builtin_expect(size == 0, 0) || (alignment & (alignment - 1)) != 0)
        return NULL;

    size = ALIGN(size, alignment);
    Block *block;

    if (size >= MMAP_THRESHOLD)
        return request_space_mmap(size, alignment);
	else 
	{
        if (!freelist) 
		{
            block = request_space_sbrk(NULL, size, alignment);
            if (__builtin_expect(!block, 0))
                return NULL;
            freelist = block;
        }
		else 
		{
            Block *last = freelist;
            block = find_free_block(&last, size, alignment);
            if (block) 
			{
                if (block->size >= size + BLOCK_SIZE) 
                    split_block(block, size);
                block->free = 0;
            }
			else 
			{
                block = request_space_sbrk(last, size, alignment);
                if (__builtin_expect(!block, 0))
                    return NULL;
            }
        }
    }

    uintptr_t addr = (uintptr_t)block->aligned_address;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    block->aligned_address = (void *)aligned_addr;
	if (alignment >= 32)
	{
		#ifdef DEBUG
			printf("Allocated memory at address %p (Block %d, aligned to %zu)\n", block->aligned_address, allocated_blocks, alignment);
		#endif
		_memset_avx(block->aligned_address, 0, size);
	}
	else
	{
		#ifdef DEBUG
			printf("Allocated memory at address %p (Block %d, aligned to %zu)\n", block->aligned_address, allocated_blocks, alignment);
		#endif
		_memset_ERMS(block->aligned_address, 0, size);
	}
	return block->aligned_address;
}

/* void *allocate_cache(size_t size) { */
/* 	void *ptr = _aligned_alloc(ALIGNMENT, size); */
/* 	if (!ptr)  */
/* 		return NULL; */
/*  */
/*     if (size <= CACHE_SIZE_L1)  */
/* 	{ */
/*         for (size_t i = 0; i < size; i += 64) */
/*             _mm_prefetch(ptr + i, _MM_HINT_T0); */
/*     }  */
/* 	else if (size <= CACHE_SIZE_L2)  */
/* 	{ */
/*         for (size_t i = 0; i < size; i += 64) */
/*             _mm_prefetch(ptr + i, _MM_HINT_T1); */
/*     }  */
/* 	else */
/* 		_mm_prefetch(ptr, _MM_HINT_NTA);   */
/* 	return ptr; */
/* } */
