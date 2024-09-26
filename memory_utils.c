#include "include.h"
#include <immintrin.h>


#define BITS 0x0101010101010101ULL

/*
* 0x0101010101010101ULL is a 64-bit constant with the value 0x01 repeated 8 times.
* This constant is used to set 8 bytes to the value c.
*
* in binary 0x01 is 0000 0001 so :
*
* 00000001 00000001 00000001 00000001 
* 00000001 00000001 00000001 00000001
*
*/

void *_memset_avx(void *s, int c, size_t n) 
{
    unsigned char *p = (unsigned char *)s;

    if (n == 0) return s;

    __m256i fill = _mm256_set1_epi8((unsigned char)c);

    size_t misalign = ((uintptr_t)p) & 31;
    size_t align_size = misalign ? (32 - misalign) : 0;

    if (align_size > n) align_size = n;

    for (size_t i = 0; i < align_size; i++) {
        *p++ = c;
        n--;
    }

    while (n >= 32) {
        _mm256_storeu_si256((__m256i *)p, fill);
        p += 32;
        n -= 32;
    }
	
	while (n >= 16) {
		_mm_storeu_si128((__m128i *)p, _mm256_castsi256_si128(fill));
		p += 16;
		n -= 16;
	}

    while (n >= 8) {
        *(uint64_t *)p = (uint64_t)(unsigned char)c * BITS;
        p += 8;
        n -= 8;
    }

    while (n--) {
		*p++ = c;
    }
    return s;
}




void *_memcpy_ERMS(void *dest, const void *src, size_t n) {
	__asm__ __volatile__ (
		"rep movsb"
		: "+D"(dest), "+S"(src), "+c"(n)
		:
		: "memory"
	);
	return dest;
}

void *_memcpy_avx(void *dest, const void *src, size_t n) {
    void *ret = dest;

    uintptr_t dest_ptr = (uintptr_t)dest;
    uintptr_t src_ptr = (uintptr_t)src;

    while (dest_ptr % 32 != 0 && n > 0) 
	{
        *(char *)dest_ptr = *(const char *)src_ptr;
        dest_ptr++;
        src_ptr++;
        n--;
    }
	#ifdef DEBUG
		printf("Copying memory using AVX\n");
		check_alignment((void *)dest_ptr);
		printf("\n");
	#endif
    while (n >= 16) 
	{
        __m128i data = _mm_loadu_si128((const __m128i *)(src_ptr));
        _mm_storeu_si128((__m128i *)(dest_ptr), data);
        src_ptr += 16;
        dest_ptr += 16;
        n -= 16;
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
