#include "include.h"

extern Block *freelist;
extern size_t block_size;
extern int allocated_blocks;
extern int freed_blocks;


#define UNIT 16

void *_aligned_alloc(size_t alignment, size_t size) {
    if ((alignment & (alignment - 1)) != 0 || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    if (alignment < UNIT) {
        alignment = UNIT;
    }

    unsigned char *p = malloc(size + alignment - 1 + sizeof(void*));
    if (!p)
        return NULL;

    uintptr_t addr = (uintptr_t)(p + sizeof(void*));
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    ((void**)aligned_addr)[-1] = p;

    return (void*)aligned_addr;
}

void _aligned_free(void *ptr) {
    if (ptr) {
        free(((void**)ptr)[-1]);
    }
	return;
}
