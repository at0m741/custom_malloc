#include "include.h"
#include <immintrin.h>

void *_memset_avx(void *s, int c, size_t n) {
    if (n == 0) return s;

    __attribute__((aligned(sizeof(__m256i)))) unsigned char *p = (unsigned char *)s;
    __m256i fill = _mm256_set1_epi8((unsigned char)c);

    size_t offset = ((uintptr_t)p) & 31;
    while (offset && n) {
        *p++ = c;
        --n;
        offset = ((uintptr_t)p) & 31;
	}

    while (n >= 32) {
        _mm256_storeu_si256((__m256i *)p, fill);
        p += 32;
        n -= 32;
    }

    while (n) {
        *p++ = c;
        --n;
    }

    return s;
}

void *_memset_ERMS(void *s, int c, size_t n) {
	__asm__ __volatile__ (
		"rep stosb"
		: "+D"(s), "+c"(n)
		: "a"(c)
		: "memory"
	);
	return s;
}

void *_memcpy_avx(void *dest, const void *src, size_t n) {
    void *ret = dest;

    uintptr_t dest_ptr = (uintptr_t)dest;
    uintptr_t src_ptr = (uintptr_t)src;

    while (!dest_ptr % 32 && n > 0) 
	{
        *(char *)dest_ptr = *(const char *)src_ptr;
        dest_ptr++;
        src_ptr++;
        n--;
    }

    if (n <= 16)
	{
        __m128i data = _mm_loadu_si128((const __m128i *)(src_ptr));
        _mm_storeu_si128((__m128i *)(dest_ptr), data);
        return ret;
    }

    while (n >= 32)
	{
        __m256i data = _mm256_loadu_si256((const __m256i *)(src_ptr));
        _mm256_storeu_si256((__m256i *)(dest_ptr), data);
        src_ptr += 32;
        dest_ptr += 32;
        n -= 32;
    }
    while (n > 0) 
	{
        *(char *)dest_ptr = *(const char *)src_ptr;
        dest_ptr++;
        src_ptr++;
        n--;
    }

    return ret;
}
