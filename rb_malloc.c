#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

enum { RED = 0, BLACK = 1 };
enum { ALLOCATED = 0, FREE = 1};

struct meta {
    size_t size;
    struct meta *l, 
                *r, 
                *p;   
    uint8_t color;            // RED/BLACK
    uint8_t state;            // FREE/ALLOCATED
};

static struct meta *root = NULL;

static void insert_rb(struct meta *node);
static struct meta *find_free(size_t size);
static struct meta *issue_space(size_t size);
static void rotate_left(struct meta *x);
static void rotate_right(struct meta *y);
static void rb_insert_fixup(struct meta *x);
static void bst_insert(struct meta *node);
static int less(struct meta *a, struct meta *b);
static void print_tree(struct meta *node, int depth);

void *rb_malloc(size_t size) {
    if (size == 0) return NULL;

    struct meta *block = NULL;

    block = find_free(size);
    if (!block) {
        block = issue_space(size);
        if (!block) return NULL;
    }
    block->state = ALLOCATED;
    return (void *)(block + 1);
}

static struct meta *issue_space(size_t size) {
    void *prev_brk = sbrk(0);
    if (prev_brk == (void *)-1) {
        return NULL;
    }

    void *req = sbrk(sizeof(struct meta) + size);
    if (req == (void *)-1) {
        return NULL;
    }
    struct meta *block = (struct meta *)prev_brk;
    block->size = size;
    block->l = block->r = block->p = NULL;
    block->color = RED;
    block->state = FREE;

    insert_rb(block);
    return block;
}

static struct meta *find_free(size_t size) {
    struct meta *curr = root, *best = NULL;
    
    while (curr) {
        if (curr->state == FREE && curr->size >= size) {
            best = curr;
            curr = curr->l;
        } else {
            curr = curr->r;
        }
    }
    return best;
}


static int less(struct meta *a, struct meta *b) {
    if (a->size != b->size) {
        return a->size < b->size;
    }
    return (uintptr_t)a < (uintptr_t)b;
}

static void bst_insert(struct meta *node) {
    if (!root) { 
        root = node; 
        node->color = BLACK; 
        return;
    }

    struct meta *parent = NULL, 
                *curr = root;
    while (curr) {
        parent = curr;
        if (less(node, curr)) {
            curr = curr->l;
        } else {
            curr = curr->r;
        }
    }
    node->p = parent;
    if (less(node, parent)) {
        parent->l = node;
    }
    else {
        parent->r = node;
    }
}

static void insert_rb(struct meta *node) {
    bst_insert(node);
    rb_insert_fixup(node);
}

static void rotate_left(struct meta *x) {
    struct meta *y = x->r;

    x->r = y->l;
    if (y->l) {
        y->l->p = x;
    }

    y->p = x->p;
    if (!x->p) {
        root = y;
    } else if (x == x->p->l) {
        x->p->l = y;
    } else {
        x->p->r = y;
    }

    y->l = x;
    x->p = y;
}

static void rotate_right(struct meta *y) {
    struct meta *x = y->l;

    y->l = x->r;
    if (x->r) {
        x->r->p = y;
    }

    x->p = y->p;
    if (!y->p) {
        root = x;
    } else if (y == y->p->l) {
        y->p->l = x;
    } else {
        y->p->r = x;
    }

    x->r = y;
    y->p = x;
}

static void rb_insert_fixup(struct meta *z) {
    while (z->p && z->p->color == RED) {
        struct meta *p = z->p;
        struct meta *g = p->p;
        if (!g) break; // parent is root

        if (p == g->l) {
            struct meta *y = g->r; // uncle
            // Case 1: uncle red -> recolor and move up
            if (y && y->color == RED) {
                p->color = BLACK;
                y->color = BLACK;
                g->color = RED;
                z = g;
            } else {
                // Case 2: zig-zag -> rotate at parent to make it zig-zig
                if (z == p->r) {
                    z = p;
                    rotate_left(z);
                    p = z->p;
                    g = p->p;
                }
                // Case 3: zig-zig -> rotate at grandparent, recolor
                p->color = BLACK;
                g->color = RED;
                rotate_right(g);
            }
        } else {
            // Symmetric (p = g->r)
            struct meta *y = g->l; // uncle
            if (y && y->color == RED) {
                p->color = BLACK;
                y->color = BLACK;
                g->color = RED;
                z = g;
            } else {
                if (z == p->l) {
                    z = p;
                    rotate_right(z);
                    p = z->p;
                    g = p->p;
                }
                p->color = BLACK;
                g->color = RED;
                rotate_left(g);
            }
        }
    }
    if (root) root->color = BLACK;
}


void print_rb_extern() {
    print_tree(root, 0);
}

static void print_tree(struct meta *node, int depth) {
    if (!node) return;

    print_tree(node->l, depth + 1);
    for (int i = 0; i < depth; ++i) {
        printf("    ");
    }
    printf("[%zu %s]\n",
           node->size,
           node->color == RED ? "R" : "B");
    print_tree(node->r, depth + 1);
}
