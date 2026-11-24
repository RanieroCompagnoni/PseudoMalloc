#include <stdio.h>
#include <string.h>
#include "pseudo_malloc.h"

#define MAX_NUMBER_NODES 512
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
        printf("Errore di Allocazione per 1023 byte; buddy = %c\n",p1);
        return 1;
    }
    char * p2 = (char*)pseudomalloc(1023);
    //test 1024 byte -> mmap
    if(p2==NULL){
        printf("Errore di Allocazione per 1024 byte; mmap = %c\n",p2);
        return 1;
    }

    memset(p1,1, 1023);
    memset(p2,2, 1024);
    pseudofree(p1);
    pseudofree(p2);
    printf("Allocati 0, 1023 e 1024 byte correttamente\n\n");

    //test allocazione albero completo con buddy (1MB e 8 livelli -> 2^8 = 256 nodi)
    char* nodi[MAX_NUMBER_NODES];
    for(int i=0; i<256; i++){
        nodi[i] = (char*)pseudomalloc(500); //richiesta piccola entra in una singola foglia -> buddy
        if(nodi[i]==NULL){
            printf("Allocazione singola foglia fallita all'indice: %d\n\n",i);
            return 1;
        }
        memset(nodi[i],i,500);
    }
    printf("Allocati 256 blocchi correttamente\n");

    //ora l'albero è completamente pieno qualsiasi altro allocazione è impossibile
    void* overflow = pseudomalloc(100);
    if (overflow != NULL) {
        printf("Errore l'albero non era stato completamente allocato\n\n");
        return 1;
    }

    //test per vedere se ci sono stati errori di ripetizione nell'allocazione 
    for(int i=0; i<256; i++){
        for(int j=0; j<500; j++){
            if(nodi[i][j] != i){
                printf("Errore , node[%d]=%d invece che %d",j,nodi[j],i);
                return 1;
            }
        }
    }

    //free tutti i nodi
    for(int i=0; i<256; i++){
        pseudofree(nodi[i]);
    }

    
    printf("\n=====Richiesta 100 byte (buddy_allocator)=====\n");
    p1 = (char*)pseudomalloc(100);
    if (p1) {
        strcpy(p1, "Allocato con buddy_allocator");
        printf(" -> Allocato a %p | Contenuto: %s\n", (void*)p1, p1);
    } else {
        printf(" -> Allocazione Fallita\n");
    }

    printf("\n=====Richiesta 5000 byte (mmap)=====\n");
    p2 = (char*)pseudomalloc(5000);
    if (p2) {
        strcpy(p2, "Allocato con mmap");
        printf(" -> Allocato a %p | Contenuto: %s\n", (void*)p2, p2);
    } else {
        printf(" -> Allocazione Fallita\n");
    }

    void * mixed[50];

    for(int i=0; i < 50; i++){
        size_t sz;
        if(i%2==0){
            sz = 256;
        }else{
            sz = 4096 * (i+1);
        }

        mixed[i]=pseudomalloc(sz);
        if(mixed[i]==NULL){
            printf("Errore nell'allocazione mista , all'indice %d con size = %zu\n\n",i,sz);
        }
        memset(mixed[i],i,sz);
    }

    for(int i=0; i< 50; i++){
        pseudofree(mixed[i]);
    }

    printf("\n=====Deallocazione memorie=====\n");
    pseudofree(p1);
    printf(" -> p1 liberato.\n");

    pseudofree(p2);
    printf(" -> p2 liberato.\n");
    printf("\n=== TEST COMPLETATI ===\n");
    
    return 0;
}