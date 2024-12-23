#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <immintrin.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <stdarg.h>

/*
	* ALIGNMENT: alignment of the block 
	* ALIGN(size, alignment): align the size to the alignment
	* MMAP_THRESHOLD: threshold to use mmap
	* MMAP_SIZE: size of the mmap block
	* MMAP_ALIGN(size): align the size to the mmap size
	* BIN_COUNT: number of bins
	* BIN_MAX_SIZE: maximum size of the bin
	* CACHE_SIZE_L1: size of the L1 cache
	* CACHE_SIZE_L2: size of the L2 cache
*/

#ifndef __GNUC__
#define __builtin_align_up(x, align) (((x) + (align - 1)) & ~(align - 1))
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
#define UNIT 16
#define BITMAP_SIZE 1024   
#define BLOCK_UNIT_SIZE 32 
#define MEMORY_POOL_SIZE (BITMAP_SIZE * BLOCK_UNIT_SIZE)
#define BLOCK_SIZE ALIGN(sizeof(Block), ALIGNMENT)
#define MAX_BLOCK_SIZE 1024 * 1024


typedef enum {
	NO_CACHE = 0,
	L1_CACHE = 1,
	L2_CACHE = 2,
	L3_CACHE = 3
} CacheLevel;

#define CACHE_LEVEL

typedef enum {
	TINY = 0,
	SMALL = 1,
	LARGE = 2	
} BinType;

/* 
	* get cache level 
	* 0: no cache
	* 1: L1 cache
	* 2: L2 cache
	* 3: L3 cache
	*  this is used to determine the cache size
*/

void get_cache_info();
void *allocate_cache(size_t size);

/*
	* this structure is used to store the block information 
	* size: size of the block
	* next: pointer to the next block
	* free: flag to indicate if the block is free
	* aligned_address: aligned address of the block
	* is_mmap: flag to indicate if the block is allocated using mmap
*/

struct group {
    struct meta *meta;
    unsigned char active_idx:5;
    char pad[UNIT - sizeof(struct meta *) - 1];
    unsigned char storage[];
};

typedef struct Block {
    size_t size;
    struct Block *next;
    int free;
	void *aligned_address;
	int is_mmap;
} Block;

typedef struct MemoryAllocator {
    Block *freelist;
    int allocated_blocks;
	size_t block_size;
    Block *bins[BIN_COUNT];
} MemoryAllocator;

__attribute__((always_inline))
static inline uintptr_t align_up(uintptr_t addr, size_t alignment) {
    return (addr + alignment - 1) & ~(alignment - 1);
}

/* memory utils */

void *_memcpy_avx(void *dest, const void *src, size_t n);
void *_memset_avx(void *s, int c, size_t n);
void *_memset_ERMS(void *s, int c, size_t n); 
void *_memcpy_ERMS(void *dest, const void *src, size_t n);

/* block utils */

void coalesce_free_blocks(); 
void *find_free_block(size_t size, size_t alignment); 
void split_block(Block *block, size_t size, size_t alignment);
void initialize_allocator();
/* memory allocation */

void *request_space_mmap(size_t size, size_t alignment);
Block *request_space(Block *last, size_t size, size_t alignment);
void check_alignment(void *aligned_address);
void *_malloc(size_t size);
void *_aligned_alloc(size_t alignment, size_t size);
void _free(void *ptr);
void _aligned_free(void *ptr); 
void *_realloc(void *ptr, size_t new_size); 
/* memory leak detection and utils */

long _syscall(long number, ...);
void check_for_leaks();
void* _sbrk(intptr_t increment);
void hexdump(void *ptr, size_t size);
int count_blocks(Block *list); 

#define __vector __attribute__((vector_size(16) ))



#endif
