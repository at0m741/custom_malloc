#include "include.h"

void *_memcpy_ERMS(void *dest, const void *src, size_t n) {
	__asm__ __volatile__ (
		"rep movsb"
		: "+D"(dest), "+S"(src), "+c"(n)
		:
		: "memory"
	);
	return dest;
}

