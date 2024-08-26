#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define DEBUG 1
#define ALIGNMENT 8
#define ALIGN(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))
#define MMAP_THRESHOLD (128 * 1024) 
#define MMAP_SIZE (128 * 1024)
#define MMAP_ALIGN(size) (((size) + (MMAP_SIZE - 1)) & ~(MMAP_SIZE - 1))


typedef struct __attribute__((aligned(ALIGNMENT))) Block {
    size_t size;
    struct Block *next;
    int free;
	void *aligned_address;
} Block;

#define BLOCK_SIZE ALIGN(sizeof(Block), ALIGNMENT)

Block *find_free_block(Block **last, size_t size, size_t alignment); 
void split_block(Block *block, size_t size);
void *request_space_mmap(size_t size, size_t alignment);
Block *request_space_sbrk(Block *last, size_t size, size_t alignment);
void check_alignment(void *aligned_address);
void coalesce_free_blocks(); 
void *_malloc(size_t size);
void _free(void *ptr);
void check_for_leaks();
void hexdump(void *ptr, size_t size);

#endif
