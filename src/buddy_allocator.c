#include <assert.h>
#include <math.h> // for floor and log2
#include <stdio.h>
#include <stdlib.h>

#include "buddy_allocator.h"
#include "bit_map.h"

// these are trivial helpers to support you in case you want
// to do a bitmap implementation
int levelIdx(size_t idx) { return (int)floor(log2(idx)); };

int buddyIdx(int idx) {
  if (idx & 0x1) {
    return idx - 1;
  }
  return idx + 1;
}

int parentIdx(int idx) { return idx / 2; }

int startIdx(int idx) { return (idx - (1 << levelIdx(idx))); }

// computes the size in bytes for the allocator
int BuddyAllocator_calcSize(int num_levels) {
  //num_nodi = (2^(num_levels+1) -1)
  int total_nodes = (1 << (num_levels+1)) - 1;
  return BitMap_getBytes(total_nodes);
}

void BuddyAllocator_init(BuddyAllocator* alloc,int num_levels,size_t memory_size, char* memory){
  assert(num_levels <= MAX_LEVELS && "Numero di livelli non valido");

  int total_nodes = (1 << (num_levels+1)) - 1;
  size_t bitmap_bytes = BitMap_getBytes(total_nodes);

  BitMap_init(&alloc->bitmap, total_nodes, (uint8_t*) memory);

  //azzero tutti i nodi della bitmap (tutti liberi)
  for (int i = 1; i <= total_nodes; ++i) {
    BitMap_setBit(&alloc->bitmap, i, 0);
  }

  alloc->num_levels = num_levels;
  //memoria totalia (albero + bitmap)
  alloc->memory = memory + bitmap_bytes;
  //memoria per albero (esattamente una potenza di 2)
  alloc->memory_size = memory_size - bitmap_bytes;

  alloc->min_bucket_size = alloc->memory_size >> num_levels;

  #ifdef _VERBOSE_
    printf("BUDDY ALLOCATOR INIZIALIZZATO\n");
    printf(" |- Livelli:             %d\n", num_levels);
    printf(" |- Nodi totali:         %d\n", total_nodes);
    printf(" |- Dimensione bitmap:   %zu byte (all'inizio del buffer)\n", bitmap_bytes);
    printf(" |- Memoria utente:      %zu byte (all'indirizzo %p)\n", alloc->memory_size, (void*)alloc->memory);
    printf(" |- Min bucket size:     %zu byte\n", alloc->min_bucket_size);
  #endif
}

int find_free_node(BuddyAllocator* alloc, int node, int current_level, int target_level) {
  //caso base -> livello giusto trovato
  if(current_level == target_level){
    if(BitMap_bit(&alloc->bitmap, node) == 0){
      return node;
    }
    return -1;
  }

  //caso nodo padre == 0 -> è completamente libero (cosi come tutti i suoi figli)
  if(BitMap_bit(&alloc->bitmap, node) == 0){
    while(current_level<target_level){
      node = node * 2;
      current_level++;
    }
    return node;
  }

  //caso nodo padre == 1 -> o è tutto pieno o è stata gia splittato
  int left_child = node * 2;
  int right_child = node * 2 + 1;

  //i figli sono entrambi a 0 , il padre era completamente allocato
  if(BitMap_bit(&alloc->bitmap, left_child) == 0 && BitMap_bit(&alloc->bitmap, right_child==0)){
    return -1;
  }

  //era solo splittato , si cerca ricorsivamente partendo dal sottoalbero sx
  int found = find_free_node(alloc, left_child, current_level + 1, target_level);
  if (found != -1){
    return found;
  }
  //albero destro
  return find_free_node(alloc, right_child, current_level + 1, target_level);
}

void* BuddyAllocator_malloc(BuddyAllocator* alloc, size_t size){
  if(size==0 || size>alloc->memory_size){
    printf("Dimensione non allocabile");
    return NULL;
  }

  //aggiungo spazio per un header , mi servirà per la free e per ritrovare il nodo nella bitmap
  int real_size = size + sizeof(int);

  /*partendo dalle foglie e risalendo cerco il livello con 
  blocchi sufficientementi grandi per allocare real size*/ 
  int target_level = alloc->num_levels;
  while (target_level > 0 && (alloc->memory_size >> target_level) < real_size){
      target_level--;
  }

  if (target_level > alloc->num_levels) target_level = alloc->num_levels;

  int free_idx = find_free_node(alloc,1,0,target_level);
  if (free_idx == -1){
    printf("Memoria Insufficiente");
    return NULL;
  }

  //settiamo a 1 il nodo scelto e tutti i suoi antenati sulla bitmap
  int current_idx = free_idx;
  while(current_idx >=1){
    BitMap_setBit(&alloc->bitmap, current_idx, 1);
    current_idx = parentIdx(current_idx);
  }


  int level = levelIdx(free_idx);
  int offset = startIdx(free_idx);
  size_t bucket_size = alloc->memory_size >> level;
  char* block_ptr = alloc->memory + (offset * bucket_size);
  *((int*)block_ptr) = free_idx;
  return (void*)(block_ptr + sizeof(int));
}

// releases allocated memory
void BuddyAllocator_free(BuddyAllocator *alloc, void *mem){
#ifdef _VERBOSE_
  printf("BuddyAllocator_free|destroying [ %p ]\n", mem);
#endif
  if (!mem) {
    printf("BuddyAllocator_free|WARINING, cannot free this memory address\n");
    return; 
  }

  char* char_mem = (char*)mem;
  int* header = (int*)(char_mem - sizeof(int));
  int idx = *header;

  BitMap_setBit(&alloc->bitmap, idx, 0);

  while (idx > 1) { 
    int buddy = buddyIdx(idx);

    if (BitMap_bit(&alloc->bitmap, buddy) == 0) {
        int p = parentIdx(idx);
        BitMap_setBit(&alloc->bitmap, p, 0);
        idx = p; 
    } else {
        break;
    }
  }
} 