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

Block *find_free_block(Block **last, size_t size, size_t alignment) {
	if (__builtin_expect(freelist == NULL, 0 || size == 0))
		return NULL;

    Block *current = freelist;
    while (current && !(current->free && current->size >= size)) 
	{
        *last = current;
        current = current->next;
		#ifdef DEBUG
			printf("Searching for free block\n");
			printf("Current block: %p\n", current);
			printf("\n");
		#endif
    }
    return current;
}

/*
	* Function to split a block into two blocks
	* block: block to be split
	* size: size of the memory to be allocated
	* this function is called when the block is larger than the requested size
	* the block is split into two blocks, one of the requested size and the other of the remaining size
*/



inline void split_block(Block *block, size_t size, size_t alignment) {
	if (__builtin_expect(block->size < size, 0))
		return;

    size_t remaining_size = block->size - size - BLOCK_SIZE;

    if (remaining_size > BLOCK_SIZE) {
        uintptr_t new_block_address = (uintptr_t)block + BLOCK_SIZE + size;
        uintptr_t aligned_new_block_address = (new_block_address + alignment - 1) & ~(alignment - 1);
        size_t wasted_space = aligned_new_block_address - new_block_address;

        remaining_size -= wasted_space;
        if (remaining_size > BLOCK_SIZE) {
            Block *new_block = (Block *)aligned_new_block_address;
            new_block->size = remaining_size;
			printf("new_block->size = %zu\n", new_block->size);
            new_block->free = 1;
            new_block->next = block->next;
            block->size = size;
            block->next = new_block;
        }
    }
#ifdef DEBUG
	printf("Splitting block\n");
	printf("Block size: %zu\n", block->size);
	printf("Remaining size: %zu\n", remaining_size);
	check_alignment(block->aligned_address);
	printf("\n");
#endif

}


/*
	* this function call sbrk to allocate memory
	* last: last block in the freelist
	* size: size of the memory to be allocated
	* alignment: alignment of the memory to be allocated
	* only be used if the requested size is larger than the block size
	* Returns: pointer to the allocated memory
*/
#include <errno.h>
#include <stdint.h>
#include <sys/syscall.h>


#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
long _syscall(long number, ...);

long _syscall(long number, ...) {
    va_list args;
    va_start(args, number);

    // Récupération des arguments
    unsigned long arg1 = va_arg(args, unsigned long);
    unsigned long arg2 = va_arg(args, unsigned long);
    unsigned long arg3 = va_arg(args, unsigned long);
    unsigned long arg4 = va_arg(args, unsigned long);
    unsigned long arg5 = va_arg(args, unsigned long);
    unsigned long arg6 = va_arg(args, unsigned long);

    va_end(args);

    long ret;

    __asm__ volatile (
        "mov %1, %%rax \n"    // numéro de syscall dans rax
        "mov %2, %%rdi \n"    // arg1 dans rdi
        "mov %3, %%rsi \n"    // arg2 dans rsi
        "mov %4, %%rdx \n"    // arg3 dans rdx
        "mov %5, %%r10 \n"    // arg4 dans r10
        "mov %6, %%r8  \n"    // arg5 dans r8
        "mov %7, %%r9  \n"    // arg6 dans r9
        "syscall        \n"    // appel système
        "mov %%rax, %0  \n"    // récupérer la valeur de retour
        : "=r" (ret)             // sortie
        : "r" (number),          // entrée
          "r" (arg1),
          "r" (arg2),
          "r" (arg3),
          "r" (arg4),
          "r" (arg5),
          "r" (arg6)
        : "%rax", "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9", "memory"
    );

    // Gestion des erreurs
    if (ret < 0) {
        // Vous pouvez définir errno ici si nécessaire
        return ret;
    }

    return ret;
}

void* _sbrk(intptr_t increment) {
    static uintptr_t current_brk = 0;
    uintptr_t new_brk;
    void* result;

    if (current_brk == 0) {
        result = (void*)_syscall(SYS_brk, 0);
        current_brk = (uintptr_t)result;
        if (current_brk == (uintptr_t)-1) {
            errno = ENOMEM;
            return (void*)-1;
        }
    }

    new_brk = current_brk + increment;
    result = (void*)_syscall(SYS_brk, new_brk);
    if ((uintptr_t)result == (uintptr_t)-1) {
        errno = ENOMEM;
        return (void*)-1;
    }

    void* old_brk = (void*)current_brk;
    current_brk = new_brk;

    return old_brk;
}


Block *request_space_sbrk(Block *last, size_t size, size_t alignment) {
    size_t total_size = size + sizeof(Block) + alignment - 1;
    void *request = _sbrk(total_size);
    if (request == (void *)-1)
        return NULL;

    uintptr_t raw_addr = (uintptr_t)request;
    uintptr_t aligned_addr = ALIGN(raw_addr + sizeof(Block), alignment);
    Block *block = (Block *)(aligned_addr - sizeof(Block));

    block->size = size;
    block->free = 0;
    block->is_mmap = 0;
    block->next = NULL;
    block->aligned_address = (void *)aligned_addr;

    if (last) {
        last->next = block;
    } else if (!freelist) {
        freelist = block;
    }

    allocated_blocks++;

    #ifdef DEBUG
        printf("Allocated memory at address %p\n", block->aligned_address);
        printf("Allocated blocks: %d\n", allocated_blocks);
        check_alignment(block->aligned_address);
        printf("\n");
		#endif

    memset(block->aligned_address, 0, size);

    return block;
}

/*
	* this function call mmap to allocate memory
	* size: size of the memory to be allocated
	* alignment: alignment of the memory to be allocated
	* Returns: pointer to the allocated memory
*/



void *request_space_mmap(size_t size, size_t alignment) {
    size_t total_size = size + BLOCK_SIZE + alignment - 1;
    void *mapped_memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mapped_memory == MAP_FAILED)
        return NULL;

    uintptr_t raw_addr = (uintptr_t)mapped_memory;
    uintptr_t aligned_addr = ALIGN(raw_addr + sizeof(Block), alignment);
    Block *block = (Block *)(aligned_addr - sizeof(Block));

    block->size = size;
    block->next = NULL;
    block->free = 0;
    block->is_mmap = 1;
    block->aligned_address = (void *)aligned_addr;

    allocated_blocks++;
    return block->aligned_address;
}


