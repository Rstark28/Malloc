#ifndef RB_MALLOC_H
#define RB_MALLOC_H

#include <stddef.h>

/**
 * @file rb_malloc.h
 * @brief Red-black tree based memory allocator.
 * @return Pointer to the allocated memory block, or NULL on failure.
 */
void *rb_malloc(size_t size);

/**
 * @brief Free a previously allocated memory block.
 * @param ptr Pointer to the memory block to free.
 * @return void
 */
void rb_free(void *ptr);

/**
 * @brief Reallocate a memory block.
 * @param ptr Pointer to the memory block to reallocate.
 * @param size New size of the memory block.
 * @return Pointer to the reallocated memory block, or NULL on failure.
 */
void *rb_realloc(void *ptr, size_t size);

/**
 * @brief Allocate a zero-initialized memory block.
 * @param count Number of elements to allocate.
 * @param size Size of each element.
 * @return Pointer to the allocated memory block, or NULL on failure.
 */
void *rb_calloc(size_t count, size_t size);

/**
 * @brief Print the contents of the red-black tree. Debugging purposes.
 * @return void
 */
void print_rb_extern();

#endif // RB_MALLOC_H