#include <stdio.h>
#include <string.h>
#include "pseudo_malloc.h"

int main(void) {
    printf("=== TESTER PSEUDOMALLOC ===\n");

    pseudomalloc_init();

    printf("\n=====Casi limiti ( 0 e PAGE_SIZE/4)=====\n");
    if(pseudomalloc(0)!=NULL){
        printf("Errore di Allocazione per 0 byte");
        return 1;
    }

    pseudofree(NULL);

    //test 1023 byte -> BuddyAllocator
    char * p1 = (char*)pseudomalloc(1023);
    if(p1==NULL){
        printf("Errore di Allocazione per 1023 byte; buddy = %c",p1);
        return 1;
    }
    char * p2 = (char*)pseudomalloc(1023);
    //test 1024 byte -> mmap
    if(p2==NULL){
        printf("Errore di Allocazione per 1024 byte; mmap = %c",p2);
        return 1;
    }

    memset(p1,1, 1023);
    memset(p2,2, 1024);
    pseudofree(p1);
    pseudofree(p2);
    printf("Allocati 0, 1023 e 1024 byte correttamente\n\n");

    printf("\n=====Richiesta 100 byte (buddy_allocator)=====\n");
    p1 = (char*)pseudomalloc(100);
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