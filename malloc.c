#include "include.h"

Block  __attribute__((visibility("hidden")))*freelist = NULL;
int  __attribute__((visibility("hidden")))allocated_blocks = 0;
size_t  __attribute__((visibility("hidden")))block_size[] = {16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 256};
Block  __attribute__((visibility("hidden")))*bins[BIN_COUNT] = {NULL};
Block *is_mmap = NULL;


/*
	* this is the custom malloc function
	* size: size of the memory to be allocated
	* Returns: pointer to the allocated memory
*/	

void *_realloc(void *ptr, size_t new_size) 
{
    if (new_size == 0) 
	{
        _free(ptr);
        return NULL;
    }
    if (ptr == NULL) 
        return _malloc(new_size);

    Block *block = (Block *)((uintptr_t)ptr - sizeof(Block));
    
    if (block->size >= new_size) 
        return ptr;

    void *new_ptr = _malloc(new_size);
    if (new_ptr == NULL)
        return NULL; 
	allocated_blocks++;
    memcpy(new_ptr, ptr, block->size);
    _free(ptr);

    return new_ptr;
}

__attribute__((hot, flatten, always_inline))
inline void *_malloc(size_t size) 
{
    if (__builtin_expect(size == 0, 0))
        return NULL;
    size = __builtin_align_up(size, ALIGNMENT); 
    Block *block = NULL;
    if (size <= BIN_MAX_SIZE) 
	{
        int bin_index = size / ALIGNMENT - 1;
        if (bin_index >= 0 && bin_index < BIN_COUNT && bins[bin_index]) 
		{
            Block *block = bins[bin_index];
            bins[bin_index] = block->next;
            block->free = 0;
            return block->aligned_address;
        }
    }

    if (size >= MMAP_THRESHOLD)
        return request_space_mmap(size, ALIGNMENT);
    else 
	{
		if (!freelist) 
		{
			block = request_space(NULL, size, ALIGNMENT);
			freelist = block;
		} 
		else 
		{
			Block *last = freelist;
			block = find_free_block(size, ALIGNMENT);
			if (block) {
				if (block->size >= size + BLOCK_SIZE)
					split_block(block, size, ALIGNMENT);
				block->free = 0;
			} 
			else
				block = request_space(last, size, ALIGNMENT);
		}
    }
    return __builtin_assume_aligned(block->aligned_address, ALIGNMENT);
}

