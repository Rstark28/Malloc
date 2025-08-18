#include <stdio.h>
#include <stdlib.h>
#include "rb_malloc.h"

int main() {
    size_t sizes[12];
    for (int i = 0; i < 12; i++) {
        sizes[i] = (rand() % 100) + 1; // random values between 1 and 100
    }
    int n = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < n; i++) {
        rb_malloc(sizes[i]);
        printf("After inserting %zu:\n", sizes[i]);
        print_rb_extern();
        printf("----\n");
    }
    printf("Sizes:\n");
    for (int i = 0; i < n; i++) {
        printf("%zu ", sizes[i]);
    }
    printf("\n");
    return 0;
}
