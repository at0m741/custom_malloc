#include "include.h"
#include <sys/mman.h>

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;

/*
	* Function to find a free block in the freelist
	* size: size of the memory to be allocated
	* alignment: alignment of the memory to be allocated
	* Returns: pointer to the allocated memory
*/


static uint32_t bitmap[BITMAP_SIZE / 32]; 
static void *memory_pool = NULL;


__attribute__((hot, always_inline))
inline void initialize_memory_pool() {
    size_t total_size = MEMORY_POOL_SIZE;
    memory_pool = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_pool == MAP_FAILED) {
        perror("mmap failed");
        memory_pool = NULL;
    } else {
        memset(bitmap, 0, sizeof(bitmap)); 
    }
}

__attribute__((hot, always_inline))
inline void *find_free_block(size_t size, size_t alignment) 
{
    if (!memory_pool || size == 0 || size > MEMORY_POOL_SIZE)
        return NULL;

    size_t units_needed = (size + BLOCK_UNIT_SIZE - 1) / BLOCK_UNIT_SIZE;

    for (size_t i = 0; i < BITMAP_SIZE; ) 
    {
        uint32_t bits = bitmap[i / 32];
        if (~bits) 
        { 
            int free_bit = __builtin_ctz(~bits);
            size_t start = (i & ~31) + free_bit;
            if (start + units_needed > BITMAP_SIZE)
                break;

            int enough_space = 1;
            for (size_t j = 0; j < units_needed; j++) 
            {
                size_t idx = start + j;
                if (bitmap[idx / 32] & (1U << (idx & 31))) 
                {
                    i = idx + 1;
                    enough_space = 0;
                    break;
                }
            }
            if (enough_space) 
            {
                for (size_t j = 0; j < units_needed; j++) 
                {
                    size_t idx = start + j;
                    bitmap[idx / 32] |= (1U << (idx & 31));
                }
                uintptr_t addr = (uintptr_t)memory_pool + start * BLOCK_UNIT_SIZE;
                uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
                return (void *)aligned_addr;
            }
        } 
        else
            i = (i & ~31) + 32;  
    }
    return NULL;
}
/*
	* Function to split a block into two blocks
	* block: block to be split
	* size: size of the memory to be allocated
	* this function is called when the block is larger than the requested size
	* the block is split into two blocks, one of the requested size and the other of the remaining size
*/

inline void split_block(Block *block, size_t size, size_t alignment) 
{
    size_t remaining_size = block->size - size - BLOCK_SIZE;
    uintptr_t new_block_address = (uintptr_t)block + BLOCK_SIZE + size;
    uintptr_t aligned_new_block_address = __builtin_align_up(new_block_address, alignment);
    
    if (remaining_size >= BLOCK_SIZE) 
	{
        Block *new_block = (Block *)aligned_new_block_address;
        new_block->size = remaining_size;
        new_block->free = 1;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}


/*
	* this function call sbrk to allocate memory
	* last: last block in the freelist
	* size: size of the memory to be allocated
	* alignment: alignment of the memory to be allocated
	* only be used if the requested size is larger than the block size
	* Returns: pointer to the allocated memory
*/





Block *request_space(Block *last, size_t size, size_t alignment) 
{
    if (__builtin_expect(size == 0, 0)) 
        return NULL;

    if ((alignment & (alignment - 1)) != 0) 
	{
        fprintf(stderr, "Error: Alignment must be a power of two.\n");
        return NULL;
    }

    size_t alignment_mask = alignment - 1;
    size_t total_size = size + sizeof(Block) + sizeof(Block *) + alignment_mask;

    size_t page_size = sysconf(_SC_PAGESIZE);
    total_size = (total_size + page_size - 1) & ~(page_size - 1);

    void *request = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (request == MAP_FAILED) 
	{
        perror("mmap failed");
        return NULL;
    }

	if (mprotect(request, total_size, PROT_READ | PROT_WRITE) == -1) 
	{
		perror("mprotect failed");
		if (munmap(request, total_size) == -1)
			perror("munmap failed");
		return NULL;
	}

    uintptr_t raw_addr = (uintptr_t)request;
    uintptr_t aligned_addr = __builtin_align_up(raw_addr + sizeof(Block) + sizeof(Block *), alignment); 

    if (aligned_addr + size > raw_addr + total_size) 
	{
        if (munmap(request, total_size) == -1)
			perror("munmap failed");
        fprintf(stderr, "Error: Aligned address exceeds allocated memory.\n");
        return NULL;
    }

    Block *block = (Block *)(raw_addr);
    block->size = size;
    block->free = 0;
    block->is_mmap = 1;
    block->next = NULL;
    block->aligned_address = (void *)aligned_addr;

    Block **back_ptr = (Block **)(aligned_addr - sizeof(Block *));
    *back_ptr = block;

    if (last)
        last->next = block;
    else if (!freelist)
        freelist = block;

    allocated_blocks++;

    return block;
}
/*
	* this function call mmap to allocate memory
	* size: size of the memory to be allocated
	* alignment: alignment of the memory to be allocated
	* Returns: pointer to the allocated memory
*/


__attribute__((hot))
void *request_space_mmap(size_t size, size_t alignment) 
{
    size_t total_size = size + BLOCK_SIZE + alignment - 1;
    void *mapped_memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (mapped_memory == MAP_FAILED)
        return NULL;
	
	if (mprotect(mapped_memory, total_size, PROT_READ | PROT_WRITE) == -1) 
	{
		perror("mprotect failed");
		munmap(mapped_memory, total_size);
		return NULL;
	}

    uintptr_t raw_addr = (uintptr_t)mapped_memory;
    uintptr_t aligned_addr = __builtin_align_up(raw_addr + BLOCK_SIZE, alignment); 
    Block *block = (Block *)(aligned_addr - sizeof(Block));

    block->size = size;
    block->next = NULL;
    block->free = 0;
    block->is_mmap = 1;
    block->aligned_address = (void *)aligned_addr;

    allocated_blocks++;
    return block->aligned_address;
}


