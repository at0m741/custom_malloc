#include "include.h"

#define UNIT 16

__attribute__((hot, flatten, always_inline))
inline void *_aligned_alloc(size_t alignment, size_t size) 
{
    if ((alignment & (alignment - 1)) != 0 || size == 0) 
	{
        errno = EINVAL;
        return NULL;
    }

    unsigned char *p = _malloc(size + alignment - 1 + sizeof(void*));
    if (!p)
        return NULL;

    uintptr_t addr = (uintptr_t)(p + sizeof(void*));
    uintptr_t aligned_addr = align_up(addr, alignment);

    ((void**)aligned_addr)[-1] = p;

    return (void*)aligned_addr;
}

__attribute__((hot, flatten, always_inline))
void _aligned_free(void *ptr) 
{
    if (ptr)
        _free(((void**)ptr)[-1]);
	return;
}
