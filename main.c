#include "rb_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
        printf("=== Demo ===\n");

        int *arr = rb_malloc(10 * sizeof(int));
        for (int i = 0; i < 10; i++) {
                arr[i] = i * i;
        }
        printf("arr[0]=%d arr[9]=%d\n", arr[0], arr[9]);

        char *reuse = rb_malloc(32);
        strcpy(reuse, "Block reuse");
        printf("reuse = %s\n", reuse);

        arr = rb_realloc(arr, 20 * sizeof(int));
        for (int i = 10; i < 20; i++) {
                arr[i] = i * i;
        }
        printf("arr[15]=%d arr[19]=%d (after grow)\n", arr[15], arr[19]);

        arr = rb_realloc(arr, 5 * sizeof(int));
        printf("arr[4]=%d (after shrink)\n", arr[4]);

        void *blocks[5];
        for (int i = 0; i < 5; i++) {
                blocks[i] = rb_malloc(16);
                printf("allocated block[%d] at %p\n", i, blocks[i]);
        }

        rb_free(blocks[1]);
        rb_free(blocks[3]);
        printf("freed block[1] and block[3]\n");

        void *x = rb_malloc(16);
        printf("new block x reused? %p\n", x);

        printf("=== Done ===\n");
        return 0;
}
