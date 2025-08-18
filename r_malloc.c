#include <assert.h>
#include <stdlib.h>

struct meta {
    size_t size;
    struct meta *l, *r;
    uint8_t free;
    uint8_t color;
};

void *root = NULL;

void *r_malloc(size_t size) {
    void *p = sbrk(0),
         *request = sbrk(size);
    if (request == (void *) -1) {
        return NULL;
    }
    assert(p == request);
    return p;
}

struct meta *find_free(size_t size) {
    struct meta *curr = root,
                *best = NULL;

    while (curr != NULL) {
        if (curr->free && curr->size >= size) {
            // Found candidate, look for better fit
            if (!best || curr->size < best->size)
                best = curr;
            curr = curr->l;
        } else {
            // Too small, go right
            curr = curr->r;
        }
    }

    return best;
}

struct meta *request_space
