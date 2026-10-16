#include <stdio.h>
#include <string.h>
#include "pseudo_malloc.h"

int main(void) {
    printf("=== TESTER PSEUDOMALLOC ===\n");

    pseudomalloc_init();

    printf("\n=====Richiesta 100 byte (buddy_allocator)=====\n");
    char* p1 = (char*)pseudomalloc(100);
    if (p1) {
        strcpy(p1, "Allocato con buddy_allocator");
        printf(" -> Allocato a %p | Contenuto: %s\n", (void*)p1, p1);
    } else {
        printf(" -> Allocazione Fallita\n");
    }

    printf("\n=====Richiesta 5000 byte (mmap)=====\n");
    char* p2 = (char*)pseudomalloc(5000);
    if (p2) {
        strcpy(p2, "Allocato con mmap");
        printf(" -> Allocato a %p | Contenuto: %s\n", (void*)p2, p2);
    } else {
        printf(" -> Allocazione Fallita\n");
    }

    // Test 3: Rilascio della memoria
    printf("\n=====Deallocazione memorie=====\n");
    pseudofree(p1);
    printf(" -> p1 liberato.\n");

    pseudofree(p2);
    printf(" -> p2 liberato.\n");
    printf("\n=== TEST COMPLETATI ===\n");
    
    return 0;
}