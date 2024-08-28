#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <immintrin.h>


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define ALIGNMENT 16
#define ALIGN(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))
#define MMAP_THRESHOLD (128 * 1024) 
#define MMAP_SIZE (128 * 1024)
#define MMAP_ALIGN(size) (((size) + (MMAP_SIZE - 1)) & ~(MMAP_SIZE - 1))
#define BIN_COUNT 10
#define BIN_MAX_SIZE 128
#define CACHE_SIZE_L1 32768
#define CACHE_SIZE_L2 262144 
/* get cache level */
void get_cache_info();
void *allocate_cache(size_t size);


typedef struct Block {
    size_t size;
    struct Block *next;
    int free;
	void *aligned_address;
	int is_mmap;
} Block;

#define BLOCK_SIZE ALIGN(sizeof(Block), ALIGNMENT)

void *_memcpy_avx(void *dest, const void *src, size_t n);
void *_memset_avx(void *s, int c, size_t n);
void *_memset_ERMS(void *s, int c, size_t n); 
Block *find_free_block(Block **last, size_t size, size_t alignment); 
void split_block(Block *block, size_t size);
void *request_space_mmap(size_t size, size_t alignment);
Block *request_space_sbrk(Block *last, size_t size, size_t alignment);
void check_alignment(void *aligned_address);
void coalesce_free_blocks(); 
void *_malloc(size_t size);
void *_aligned_alloc(size_t alignment, size_t size);
void _free(void *ptr);
void check_for_leaks();
void hexdump(void *ptr, size_t size);

#endif
