#ifndef MALLOC_H
#define MALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <immintrin.h>
#include <sys/mman.h>
#include <unistd.h>

#include <time.h>
#define NUM_SMALL_ALLOCS 1000
#define NUM_BINS 64
#define NUM_LARGE_ALLOCS 100
#define MAX_ALLOCATION_SIZE (1 << 20)
#define BLOCK_UNIT_SIZE 1024
#define MEMORY_POOL_SIZE (1024 * 1024 * 1024)
#define BITMAP_SIZE (MEMORY_POOL_SIZE / BLOCK_UNIT_SIZE)
#define ALIGNMENT 32
#define BLOCK_SIZE 4096
#define MMAP_THRESHOLD (512 * 1024)

typedef struct __attribute__((aligned(ALIGNMENT))) Block {
    size_t size;
    int _free;
    int is_mmap;
    struct Block *next;
    void *aligned_address;
} Block;


void *_malloc(size_t size);
void _free(void *ptr);
void *_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

inline uintptr_t align_up(uintptr_t addr, size_t alignment) {
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

#define LOG_DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif // MALLOC_H
