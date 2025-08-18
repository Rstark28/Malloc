#ifndef RB_MALLOC_H
#define RB_MALLOC_H

#include <stddef.h>

void *rb_malloc(size_t size);
void rb_free(void *ptr);
void *rb_realloc(void *ptr, size_t size);
void *rb_calloc(size_t count, size_t size);
void print_rb_extern();

#endif // RB_MALLOC_H