#include "pseudo_malloc.h"
#include "buddy_allocator.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#define PAGE_SIZE 4096
#define TOTAL_SIZE 1024*1024 //1MB
#define NUM_LEVELS 8

static BuddyAllocator alloc;
static char* memory_pool = NULL;

void pseudomalloc_init(void) {
    if (memory_pool != NULL) {
        return;
    }
    memory_pool = (char*)mmap(NULL,TOTAL_SIZE,PROT_READ | PROT_WRITE, -1, 0);
    
    BuddyAllocator_init(&alloc, NUM_LEVELS, TOTAL_SIZE, memory_pool)
}
void* pseudomalloc(size_t size) {
    if(size<=0){
        printf("Dimensione non valida\n");
        return NULL;
    }
    //richiesta piccola
    if(size < (PAGE_SIZE/4)){
        return BuddyAllocator_malloc(&alloc,size);
    }

    //richesta grande --> direttamente con mmap

    char* ptr = (char*)mmap(NULL, total_size,PROT_READ | PROT_WRITE, -1, 0);

    if (ptr == MAP_FAILED) {
        printf("Allocazione con mmap fallita\n");
        return NULL;
    }

    return (void*) ptr;
}
void pseudofree(void* ptr){
    if(ptr==NULL){
        return;
    }

    //controlla se l'area di memoria puntata è gestita da un buddy
    if((char*)ptr>=memory_pool && (char*)ptr<(memory_pool+TOTAL_SIZE)){
        BuddyAllocator_free(&alloc, ptr);
    }else{//zona grande gestita con mmap
        size_t size_ptr = *(size_t*)ptr;
        munmap((char*)ptr, size_ptr)
    }
}