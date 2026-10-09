#pragma once

#define MAX_LEVELS 16

#include "bit_map.h"

typedef struct  {
  BitMap bitmap; //mappa dei bit liberi / occupati
  int num_levels; //numero livelli albero
  char* memory; // memoria da gestire (puntatore a inizio memoria)
  size_t min_bucket_size; // dimensione minima blocco ritornabile
  size_t memory_size;
} BuddyAllocator;

/*rispetto al codice del corso ho eliminato tutti i riferimenti 
alle strutture liste e al all'allocatore che serviva per queste strutture
infatti afesso non viene più creata una lista per ogni livello dell'albero
ma i blocchi liberi o no verrrano memorizzati tramite BitMap e ricercati
 tramite  aritmetica dell'albero
*/

// computes the size in bytes for the buffer of the allocator
int BuddyAllocator_calcSize(int num_levels);


// initializes the buddy allocator, and checks that the buffer is large enough
void BuddyAllocator_init(BuddyAllocator* alloc,
                        int num_levels,
                        size_t memory_size, 
                        char* memory);

//allocates memory
void* BuddyAllocator_malloc(BuddyAllocator* alloc, size_t size);

//releases allocated memory
void BuddyAllocator_free(BuddyAllocator* alloc, void* mem);
