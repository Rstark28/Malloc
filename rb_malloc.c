#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { RED = 0, BLACK = 1 };
enum { ALLOCATED = 0, FREE = 1 };

struct meta {
        size_t size;
        struct meta *l, *r, *p;
        uint8_t color; // RED/BLACK
        uint8_t state; // FREE/ALLOCATED
};

static struct meta *root = NULL;

/* ---------- Forward decls ---------- */
static struct meta *find_free(size_t size);
static struct meta *issue_space(size_t size);
static void insert_rb(struct meta *node);
static void rb_insert_fixup(struct meta *z);
static void rotate_left(struct meta *x);
static void rotate_right(struct meta *y);
static void delete_rb(struct meta *z);
static void rb_delete_fixup(struct meta *x, struct meta *x_parent);
static void rb_transplant(struct meta *u, struct meta *v);
static struct meta *tree_min(struct meta *x);
static int less(struct meta *a, struct meta *b);
static void print_tree(struct meta *node, int depth);

/*
 * @brief Allocate a memory block.
 * @param size Size of the memory block to allocate.
 * @return Pointer to the allocated memory block, or NULL on failure.
 */
void *rb_malloc(size_t size)
{
        if (size == 0) {
                return NULL;
        }

        // Bump to next multiple of pointer size for alignment
        size = (size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);

        // Find a free block, if none issue space
        struct meta *block = find_free(size);
        if (!block) {
                block = issue_space(size);
                if (!block) {
                        return NULL;
                }
        }
        block->state = ALLOCATED;
        return (void *)(block + 1);
}

/*
 * @brief Free a previously allocated memory block.
 * @param ptr Pointer to the memory block to free.
 */
void rb_free(void *ptr)
{
        if (!ptr) {
                return;
        }
        struct meta *block = ((struct meta *)ptr) - 1;
        if (block->state != ALLOCATED) {
                return; // safety
        }
        block->state = FREE;
        insert_rb(block);
}

/*
 * @brief Reallocate a memory block.
 * @param ptr Pointer to the memory block to reallocate.
 * @param size New size of the memory block.
 * @return Pointer to the reallocated memory block, or NULL on failure.
 */
void *rb_realloc(void *ptr, size_t size)
{
        if (size == 0) {
                rb_free(ptr);
                return NULL;
        }
        if (!ptr) {
                return rb_malloc(size);
        }
        struct meta *block = ((struct meta *)ptr) - 1;
        if (block->state != ALLOCATED) {
                return NULL;
        }

        // If the new size is smaller return the same pointer
        if (size <= block->size) {
                return ptr;
        }

        // Otherwise, allocate a new block and copy the old data
        void *new_ptr = rb_malloc(size);
        if (!new_ptr) {
                return NULL;
        }
        memcpy(new_ptr, ptr, block->size);
        rb_free(ptr);
        return new_ptr;
}

/*
 * @brief Allocate a zero-initialized memory block.
 * @param count Number of elements to allocate.
 * @param size Size of each element.
 * @return Pointer to the allocated memory block, or NULL on failure.
 */
void *rb_calloc(size_t count, size_t size)
{
        if (count == 0 || size == 0) {
                return NULL;
        }
        size_t total_size = count * size;
        void *ptr = rb_malloc(total_size);
        if (!ptr) {
                return NULL;
        }
        memset(ptr, 0, total_size);
        return ptr;
}

/*
 * @brief Find a free block of memory.
 * @param need Size of the memory block needed.
 * @return Pointer to a free block, or NULL if none found.
 */
static struct meta *find_free(size_t need)
{
        struct meta *curr = root, *best = NULL;
        // Finds best fit block
        while (curr) {
                if (curr->size >= need) {
                        // If it's free and fits, check if it's the best fit
                        best = curr;
                        curr = curr->l;
                } else {
                        // If it doesn't fit, go right
                        curr = curr->r;
                }
        }
        // Remove best fit from tree, if found
        if (best) {
                delete_rb(best);
                best->l = best->r = best->p = NULL;
        }
        return best;
}

/*
 * @brief Allocate space for a new memory block.
 * @param size Size of the memory block to allocate.
 * @return Pointer to the new memory block (including meta), or NULL on failure.
 */
static struct meta *issue_space(size_t size)
{
        void *prev_brk = sbrk(0);
        if (prev_brk == (void *)-1) {
                return NULL;
        }
        if (sbrk(sizeof(struct meta) + size) == (void *)-1) {
                return NULL;
        }

        struct meta *block = (struct meta *)prev_brk;
        block->size = size;
        block->l = block->r = block->p = NULL;
        block->color = RED;
        block->state = ALLOCATED;
        return block;
}

static int less(struct meta *a, struct meta *b)
{
        if (a->size != b->size) {
                return a->size < b->size;
        }
        return (uintptr_t)a < (uintptr_t)b;
}

/*
 * @brief Insert a new block into the red-black tree.
 * @param node Pointer to the meta block to insert.
 */
static void insert_rb(struct meta *node)
{
        node->l = node->r = NULL;
        node->color = RED;

        // If tree is empty, set as root
        if (!root) {
                root = node;
                node->p = NULL;
                root->color = BLACK;
                return;
        }

        // Find the correct position for the new node
        struct meta *parent = NULL, *curr = root;
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
        } else {
                parent->r = node;
        }

        // Fix any red-red violations
        rb_insert_fixup(node);
}

/*
 * @brief Fix the red-black tree after insertion.
 * @param z Pointer to the newly inserted node.
 */
static void rb_insert_fixup(struct meta *z)
{
        while (z->p && z->p->color == RED) {
                struct meta *p = z->p;
                struct meta *g = p->p;
                if (!g) {
                        break; // i.e., parent is root
                }
                if (p == g->l) {               // p is left child
                        struct meta *y = g->r; // "uncle"
                        // CASE 1
                        if (y && y->color == RED) {
                                p->color = BLACK; // color flip
                                y->color = BLACK;
                                g->color = RED;
                                z = g; // move z up to grandparent
                        } else {
                                // CASE 2
                                if (z == p->r) {
                                        z = p;
                                        rotate_left(
                                            z); // rotate to make z a left child
                                        p = z->p; // update p and g for case 3
                                        g = p ? p->p : NULL;
                                }
                                // CASE 3
                                if (p && g) {
                                        p->color = BLACK; // color flip
                                        g->color = RED;
                                        rotate_right(g); // rotate to fix tree
                                }
                        }
                } else { // symmetric, p is right child
                        struct meta *y = g->l;
                        // CASE 1
                        if (y && y->color == RED) {
                                p->color = BLACK;
                                y->color = BLACK;
                                g->color = RED;
                                z = g;
                        } else {
                                // CASE 2
                                if (z == p->l) {
                                        z = p;
                                        rotate_right(z);
                                        p = z->p;
                                        g = p ? p->p : NULL;
                                }
                                // CASE 3
                                if (p && g) {
                                        p->color = BLACK;
                                        g->color = RED;
                                        rotate_left(g);
                                }
                        }
                }
        }
        if (root) {
                root->color = BLACK;
        }
}
/*
 * @brief Rotate the subtree rooted at x to the left.
 * @param x Pointer to the root of the subtree to rotate.
 */
static void rotate_left(struct meta *x)
{
        struct meta *y = x->r;
        assert(y);

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

/*
 * @brief Rotate the subtree rooted at y to the right.
 * @param y Pointer to the root of the subtree to rotate.
 */
static void rotate_right(struct meta *y)
{
        struct meta *x = y->l;
        assert(x);

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

/*
 * @brief Find the minimum node in a subtree.
 * @param x Pointer to the root of the subtree.
 * @return Pointer to the minimum node.
 */
static struct meta *tree_min(struct meta *x)
{
        while (x && x->l) {
                x = x->l;
        }
        return x;
}

/*
 * @brief Transplant one subtree into another.
 * @param u Pointer to the node to be replaced.
 * @param v Pointer to the node to replace with.
 */
static void rb_transplant(struct meta *u, struct meta *v)
{
        if (!u->p) {
                root = v;
        } else if (u == u->p->l) {
                u->p->l = v;
        } else {
                u->p->r = v;
        }
        if (v) {
                v->p = u->p;
        }
}

/*
 * @brief Delete a node from the red-black tree.
 * @param z Pointer to the node to delete.
 */
static void delete_rb(struct meta *z)
{
        if (!z) {
                return;
        }

        struct meta *y = z;
        uint8_t y_orig = y->color;
        struct meta *x = NULL;
        struct meta *x_parent = NULL;

        if (!z->l) { // if no left child, transplant right child
                x = z->r;
                x_parent = z->p;
                rb_transplant(z, z->r);
        } else if (!z->r) { // if no right child, transplant left child
                x = z->l;
                x_parent = z->p;
                rb_transplant(z, z->l);
        } else { // if both children exist, find the minimum in the right
                 // subtree
                y = tree_min(z->r);
                y_orig = y->color;
                x = y->r;

                if (y->p == z) {
                        x_parent = y;
                        if (x) {
                                x->p = y;
                        }
                } else {
                        rb_transplant(y, y->r);
                        if (y->r) {
                                y->r->p = y->p;
                        }
                        y->r = z->r;
                        y->r->p = y;
                        x_parent = y->p;
                }

                rb_transplant(z, y);
                y->l = z->l;
                y->l->p = y;
                y->color = z->color;
        }

        if (y_orig == BLACK) {
                rb_delete_fixup(x, x_parent);
        }
}

static int is_black(struct meta *n)
{
        return !n || n->color == BLACK; // NULL is considered black
}
static int is_red(struct meta *n)
{
        return n && n->color == RED;
}

/* @brief Fix the red-black tree after deletion.
 * @param x Pointer to the node to fix.
 * @param x_parent Pointer to the parent of x.
 * CREDIT to w3schools for the fixup algorithm.
 */
static void rb_delete_fixup(struct meta *x, struct meta *x_parent)
{
        while ((x != root) && is_black(x)) {
                if (x == (x_parent ? x_parent->l : NULL)) { // x is left child
                        struct meta *w =
                            x_parent ? x_parent->r : NULL; // "sibling"
                        // CASE 1: w is red
                        if (is_red(w)) {
                                w->color = BLACK;
                                x_parent->color = RED;
                                rotate_left(x_parent);
                                w = x_parent->r;
                        }
                        // CASE 2: w is black and both children are black
                        if (is_black(w ? w->l : NULL) &&
                            is_black(w ? w->r : NULL)) {
                                if (w) {
                                        w->color = RED; // color flip
                                }
                                x = x_parent; // move up
                                x_parent = x ? x->p : NULL;
                        } else {
                                if (is_black(w ? w->r
                                               : NULL)) { // CASE 3: w's right
                                                          // child is black
                                        if (w && w->l) {
                                                w->l->color = BLACK;
                                        }
                                        if (w) {
                                                w->color = RED;
                                        }
                                        if (w) {
                                                rotate_right(w);
                                        }
                                        w = x_parent ? x_parent->r : NULL;
                                }
                                if (w) { // CASE 4: w's left child is red
                                        w->color =
                                            x_parent ? x_parent->color : BLACK;
                                }
                                if (x_parent) {
                                        x_parent->color = BLACK;
                                }
                                if (w && w->r) {
                                        w->r->color = BLACK;
                                }
                                if (x_parent) {
                                        rotate_left(x_parent);
                                }
                                x = root;
                                x_parent = NULL;
                        }
                } else { // Symmetric case, x is right child
                        struct meta *w = x_parent ? x_parent->l : NULL;
                        if (is_red(w)) {
                                w->color = BLACK;
                                x_parent->color = RED;
                                rotate_right(x_parent);
                                w = x_parent ? x_parent->l : NULL;
                        }
                        if (is_black(w ? w->l : NULL) &&
                            is_black(w ? w->r : NULL)) {
                                if (w) {
                                        w->color = RED;
                                }
                                x = x_parent;
                                x_parent = x ? x->p : NULL;
                        } else {
                                if (is_black(w ? w->l : NULL)) {
                                        if (w && w->r) {
                                                w->r->color = BLACK;
                                        }
                                        if (w) {
                                                w->color = RED;
                                        }
                                        if (w) {
                                                rotate_left(w);
                                        }
                                        w = x_parent ? x_parent->l : NULL;
                                }
                                if (w) {
                                        w->color =
                                            x_parent ? x_parent->color : BLACK;
                                }
                                if (x_parent) {
                                        x_parent->color = BLACK;
                                }
                                if (w && w->l) {
                                        w->l->color = BLACK;
                                }
                                if (x_parent) {
                                        rotate_right(x_parent);
                                }
                                x = root;
                                x_parent = NULL;
                        }
                }
        }
        if (x) {
                x->color = BLACK;
        }
}

/* ---------- Debug ---------- */
void print_rb_extern(void)
{
        print_tree(root, 0);
}

static void print_tree(struct meta *node, int depth)
{
        if (!node) {
                return;
        }
        print_tree(node->l, depth + 1);
        for (int i = 0; i < depth; ++i) {
                printf("    ");
        }
        printf("[%zu %s]\n", node->size, node->color == RED ? "R" : "B");
        print_tree(node->r, depth + 1);
}
