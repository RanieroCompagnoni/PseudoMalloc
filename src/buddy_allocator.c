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

  BitMap_init(&alloc->bitmap, total_nodes, uint8_t* memory);

  memset(memory, 0, bitmap_bytes)

  alloc->num_levels = num_levels + 1;
  alloc->memory = memory + bitmap_bytes;
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

void* BuddyAllocator_malloc(BuddyAllocator* alloc, size_t size){
  
}

/*

// ----------------------------------------------------------------------- //
// ----------------------------------------------------------------------- //
//ia novel initialization function, just a single buffer also used by the pool allocator
void BuddyAllocator_initSingleBuffer(BuddyAllocator* b_allocator_,
                                     int max_num_levels_,
                                     uint64_t allocator_memory_size_,
                                     char* allocator_memory_) {
  assert(b_allocator_ && "BuddyAllocator_initSingleBuffer|ERROR, invalid allocator");
  assert(allocator_memory_size_ != 0 && allocator_memory_ &&
         "BuddyAllocator_initSingleBuffer|ERROR, invalid allocator memory");

  //ia total number of nodes in the tree
  const uint64_t total_nodes = pow(2, max_num_levels_ + 1) - 1;
  //ia compute the amount of memory required to store the
  //ia internal structures of the allocator (aka the pool allocator memory)
  const uint64_t internal_memory_required = ((sizeof(BuddyListItem) + sizeof(int))) * total_nodes;

  //ia compute the memory available to the user
  const uint64_t user_memory = (allocator_memory_size_ - internal_memory_required);
  //ia compute the minimum amount of memory that can be allocated with this levels
  const int64_t minimum_bucket_size = (user_memory >> (max_num_levels_));
  
  //ia given the number of levels and the minimum size of the buckets,
  //ia compute the amount of user memory required to work properly
  /* const uint64_t maximum_bucket_number = (1 << (max_num_levels_ + 1)); */

  //ia total number of leafs (of size minimum_bucket_size)
  const uint64_t leaf_number = (user_memory / minimum_bucket_size);

  //ia compute memory required to work properly
  const uint64_t user_memory_required = minimum_bucket_size * leaf_number;
  const uint64_t total_memory_required = internal_memory_required + user_memory_required;

  //ia log something
#ifdef _VERBOSE_
  printf("BuddyAllocator_initSingleBuffer|current configuration\n");
  printf(" |- memory avalilable ....... [ %ld ]\n",allocator_memory_size_);
  printf(" |- tree levels ............. [ %02d ]\n",max_num_levels_);
  printf(" |- tree nodes .............. [ %ld ]\n",total_nodes);
  printf(" |- leaf number ............. [ %ld ]\n",leaf_number);
  printf(" |- minimum-bs .............. [ %ld ]\n",minimum_bucket_size);
  printf(" |- total internal memory ... [ %ld ]\n",internal_memory_required);
  printf(" |- total user memory ....... [ %ld ]\n",user_memory_required);
  printf(" |- total required memory ... [ %ld ]\n",total_memory_required);
  printf("BuddyAllocator_initSingleBuffer|---------------------\n");
#endif
  
  //ia if we do not have enough memory, throw an error
  if (total_memory_required > allocator_memory_size_) {
    printf("BuddyAllocator_initSingleBuffer|ERROR, invalid configuration\n");
    exit(EXIT_FAILURE);
  }

  //ia good we have enough room for this allocator, let's do the magic
  printf("BuddyAllocator_initSingleBuffer|configuration valid, constructing allocator\n");

  //ia keep track of the initial address of the memory
  /* const char* memory_start_addr = &allocator_memory_[0]; */

  //ia give memory to the slab allocator
  char *list_start = allocator_memory_;
  PoolAllocatorResult init_result =
      PoolAllocator_init(&b_allocator_->list_allocator, sizeof(BuddyListItem),
                         total_nodes, list_start, internal_memory_required);
  printf("BuddyAllocator_initSingleBuffer|pool allocator result [ %s ]\n",
         PoolAllocator_strerror(init_result));

  // we initialize all lists
  for (int i = 0; i < MAX_LEVELS; ++i) {
    List_init(&b_allocator_->free[i]);
  }

  //ia we populate the fields of the buddy allocator
  b_allocator_->num_levels = max_num_levels_;
  b_allocator_->min_bucket_size = minimum_bucket_size;
  b_allocator_->memory = &allocator_memory_[0] + internal_memory_required;

  // we allocate a list_item to mark that there is one "materialized" list
  // in the first block
  BuddyAllocator_createListItem(b_allocator_, 1, 0);
  
 }


BuddyListItem *BuddyAllocator_getBuddy(BuddyAllocator *alloc, int level) {
  if (level < 0)
    return 0;
  assert(level <= alloc->num_levels);

  if (!alloc->free[level].size) { // no buddies on this level
    BuddyListItem *parent_ptr = BuddyAllocator_getBuddy(alloc, level - 1);
    if (!parent_ptr)
      return 0;

    // parent already detached from free list
    int left_idx = parent_ptr->idx << 1;
    int right_idx = left_idx + 1;
#ifdef _VERBOSE_
    printf("BuddyAllocator_getBuddy|split l:%d, left_idx: %d, right_idx: %d\n",
           level, left_idx, right_idx);
#endif
    BuddyListItem *left_ptr =
        BuddyAllocator_createListItem(alloc, left_idx, parent_ptr);
    BuddyListItem *right_ptr =
        BuddyAllocator_createListItem(alloc, right_idx, parent_ptr);
    // we need to update the buddy ptrs
    left_ptr->buddy_ptr = right_ptr;
    right_ptr->buddy_ptr = left_ptr;
  }
  // we detach the first
  if (alloc->free[level].size) {
    BuddyListItem *item = (BuddyListItem *)List_popFront(alloc->free + level);
    return item;
  }
  assert(0);
  return 0;
}

void BuddyAllocator_releaseBuddy(BuddyAllocator *alloc, BuddyListItem *item) {
  BuddyListItem *parent_ptr = item->parent_ptr;
  BuddyListItem *buddy_ptr = item->buddy_ptr;

  // buddy back in the free list of its level
  List_pushFront(&alloc->free[item->level], (ListItem *)item);

  // if on top of the chain, do nothing
  if (!parent_ptr)
    return;

  // if the buddy of this item is not free, we do nothing
  if (buddy_ptr->list.prev == 0 && buddy_ptr->list.next == 0)
    return;

  // join
  // 1. we destroy the two buddies in the free list;
#ifdef _VERBOSE_
  printf("BuddyAllocator_releaseBuddy|merging level %d\n", item->level);
#endif
  BuddyAllocator_destroyListItem(alloc, item);
  BuddyAllocator_destroyListItem(alloc, buddy_ptr);
  // 2. we release the parent
  BuddyAllocator_releaseBuddy(alloc, parent_ptr);
}

// allocates memory
void *BuddyAllocator_malloc(BuddyAllocator *alloc, int size) {
  // we determine the level of the page
  int mem_size = (1 << alloc->num_levels) * alloc->min_bucket_size;
  int level = floor(log2(mem_size / (size + 8)));

  // if the level is too small, we pad it to max
  if (level > alloc->num_levels)
    level = alloc->num_levels;
#ifdef _VERBOSE_
  printf("BuddyAllocator_malloc|requested: [ %d ] bytes -- level [ %d ] \n", size, level);
#endif

  // we get a buddy of that size;
  BuddyListItem *buddy = BuddyAllocator_getBuddy(alloc, level);
  //ia we were not able to allocate memory man :)
  if (!buddy) {
    printf("BuddyAllocator_malloc|WARINING, cannot allocate memory\n");
    /* return 0; */
  }

  // we write in the memory region managed the buddy address
  BuddyListItem **target = (BuddyListItem **)(buddy->start);
  *target = buddy;
  return buddy->start + 8;
}
// releases allocated memory
void BuddyAllocator_free(BuddyAllocator *alloc, void *mem) {
#ifdef _VERBOSE_
  printf("BuddyAllocator_free|destroying [ %p ]\n", mem);
#endif
  if (!mem) {
    printf("BuddyAllocator_free|WARINING, cannot free this memory address\n");
    /* return; */
  }
  // we retrieve the buddy from the system
  char *p = (char *)mem;
  p = p - 8;
  BuddyListItem **buddy_ptr = (BuddyListItem **)p;
  BuddyListItem *buddy = *buddy_ptr;
  // printf("level %d", buddy->level);
  // sanity check;
  assert(buddy->start == p);
  BuddyAllocator_releaseBuddy(alloc, buddy);
}
*/ 