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
  alloc->memory = memory + bitmap_bytes;
  alloc->memory_size = memory_size;

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

void* BuddyAllocator_malloc(BuddyAllocator* alloc, size_t size){
  if(size==0 || size>alloc->memory_size){
    printf("Dimensione non allocabile");
    return NULL;
  }

  int real_size = size + sizeof(int);

  //logaritmo livello massimo - log arrotondato per eccesso di grandezza necessaria
  int target_level = (int)log2(alloc->memory_size) - (int)ceil(log2(real_size));

  if (target_level > alloc->num_levels) target_level = alloc->num_levels;

  int current_level = 0;
  int current_node = 1;
  int free_idx = -1;

  while(current_level>=0 && current_level<=target_level){

      if(BitMap_bit(&alloc->bitmap,current_node) == 0){ //blocco libero
        if(current_level==target_level){     //se sono al livello giusto
          free_idx = current_node;    //salvo l'indice
          break;
        }else{ //il blocco era libero ma non era al livello giusto (troppo grande), allora splitto
          BitMap_setBit(&alloc->bitmap, current_node, 1); //segno come allocato
          current_node = current_node * 2;
          current_level++;
        }
      }else{  //blocco occupato o gia splittato
        int allocated_flag = 0;
        if(current_level == alloc->num_levels){ //siamo alle foglie allora devono essere sicuramente allocate
          allocated_flag=1;
        }else{ //se un nodo ha 1 sulla bitmap e entrambi i figli sono a 0 significa che è allocato 
          int left_child = current_node * 2;
          int right_child = current_node * 2 + 1;
          if (BitMap_bit(&alloc->bitmap, left_child) == 0 && BitMap_bit(&alloc->bitmap, right_child) == 0) {
              allocated_flag = 1;
          }
        }
        if(allocated_flag==0 && current_level<target_level){ // allora il blocco ha 1 sulla bitmap ma è solo splittato non completamente allocato, nel suo sottoalbero ci sarà un blocco libero
          current_node = current_node * 2;
          current_level++;
        }else{
          if(current_node % 2 != 0 || current_node == 1){  
                while((current_node % 2 != 0) && current_node > 1) {
                  current_node = current_node / 2;
                  current_level--;
                }
                if(current_node == 1) break;
          }
        }
        current_node++;
    }
  }
  if(free_idx==-1) {
    printf("Memoria non sufficiente\n");
    return NULL;
  }

  BitMap_setBit(&alloc->bitmap, free_idx, 1);
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
    /* return; */
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