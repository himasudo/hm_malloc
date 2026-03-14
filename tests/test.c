#include "../include/hm_malloc.h"
#include <stdio.h>

int main(void) {
    printf("=== hm Malloc Allocator Test Suite ===\n\n");
    
    printf("Test 1: Basic allocation\n");
    int *p1 = (int *)hm_malloc(sizeof(int));
    if (p1 == NULL) {
        printf("FAILED: malloc returned NULL\n");
        return 1;
    }
    *p1 = 42;
    printf("Allocated %zu bytes, stored value: %d\n", sizeof(int), *p1);
    print_free_list();
    
    printf("Test 2: Multiple allocations\n");
    int *p2 = (int *)hm_malloc(sizeof(int) * 10);
    char *p3 = (char *)hm_malloc(100);
    double *p4 = (double *)hm_malloc(sizeof(double) * 5);
    
    if (p2 == NULL || p3 == NULL || p4 == NULL) {
        printf("FAILED: malloc returned NULL\n");
        return 1;
    }
    
    for (int i = 0; i < 10; i++) {
        p2[i] = i * 10;
    }
    printf("Allocated multiple blocks\n");
    printf("p1=%p, p2=%p, p3=%p, p4=%p\n", (void*)p1, (void*)p2, (void*)p3, (void*)p4);
    print_free_list();
    
    printf("Test 3: Free and reuse\n");
    hm_free(p2);
    printf("Freed p2\n");
    print_free_list();
    
    int *p5 = (int *)hm_malloc(sizeof(int) * 5);
    if (p5 == NULL) {
        printf("FAILED: malloc after free returned NULL\n");
        return 1;
    }
    printf("Reallocated (should reuse freed block)\n");
    printf("p5=%p (compare with p2=%p)\n", (void*)p5, (void*)p2);
    print_free_list();
    
    printf("Test 4: Coalescing adjacent blocks\n");
    hm_free(p1);
    printf("Freed p1\n");
    print_free_list();
    
    hm_free(p5);
    printf("Freed p5 (should coalesce with p1 if adjacent)\n");
    print_free_list();
    
    printf("Test 5: Block splitting\n");
    int *p6 = (int *)hm_malloc(16);
    printf("Allocated 16 bytes\n");
    print_free_list();
    
    printf("Test 6: Free remaining blocks\n");
    hm_free(p3);
    printf("Freed p3\n");
    print_free_list();
    
    hm_free(p4);
    printf("Freed p4\n");
    print_free_list();
    
    hm_free(p6);
    printf("Freed p6 (should coalesce everything)\n");
    print_free_list();
    
    printf("Test 7: Alignment test\n");
    void *p7 = hm_malloc(13);
    if (p7 == NULL) {
        printf("FAILED: malloc returned NULL\n");
        return 1;
    }
    printf("Allocated 13 bytes (aligned to 16)\n");
    print_free_list();
    hm_free(p7);
    
    printf("\nTest 8: Edge case - malloc(0)\n");
    void *p8 = hm_malloc(0);
    if (p8 == NULL) {
        printf("malloc(0) returned NULL\n");
    } else {
        printf("malloc(0) returned %p\n", p8);
        hm_free(p8);
    }
    
    printf("\nTest 9: Edge case - free(NULL)\n");
    hm_free(NULL);
    printf("free(NULL) didn't crash\n");
    
    printf("\nTest 10: Large allocation\n");
    void *p9 = hm_malloc(10000);
    if (p9 == NULL) {
        printf("FAILED: large malloc returned NULL\n");
        return 1;
    }
    printf("Allocated 10000 bytes at %p\n", p9);
    print_free_list();
    hm_free(p9);
    print_free_list();
    
    printf("\n=== All Tests Completed ===\n");
    return 0;
}