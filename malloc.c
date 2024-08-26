#include "include.h"

Block *freelist = NULL;
int allocated_blocks = 0;
int freed_blocks = 0;

void *_malloc(size_t size) {
    if (__builtin_expect(size == 0, 0))
		return NULL;

    size = ALIGN(size, ALIGNMENT);
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
					return NULL;}
        }
    }

    return block->aligned_address;
}

void *_aligned_alloc(size_t alignment, size_t size) {
	if (__builtin_expect(size == 0, 0))
		return NULL;

	size = ALIGN(size, ALIGNMENT);
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
					return NULL;}
		}
	}

	return block->aligned_address;
}
