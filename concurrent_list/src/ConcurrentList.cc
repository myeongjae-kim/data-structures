#include "ConcurrentList.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#define DBG_PREALLOC true
#define DBG_DEALLOC true

#define SLEEP_DELAY 1000000


// Where is the location of OBSOLETE?
// A variable, or a bit of next pointer?
// If the second option is correct, we must have pred and curr node
// to mark the OBSOLETE bit.

ConcurrentList::ConcurrentList() {
  n_size = 0;
  head = (node_t*)calloc(1, sizeof(node_t)); // don't allocate it from the pool

  // checking initialization of new_node
  assert(head->next == nullptr);
  assert(head->status == INVALID);
  assert(head->elem == 0);

  head->status = HEAD; // Dummy head

  tail = head;

  initIndexArray();
}

ConcurrentList::~ConcurrentList() {
  destroyIndexArray();
  free(head);
}

// Insert a node to a list.
// First, update tail node using atomic instruction.
// Second, connect old_tail to new tail.
// Third, call next_pointer_update() to skip OBSOLETE nodes.
ConcurrentList::node_t* ConcurrentList::push_back(int elem) {
  // node_t* new_node = new node_t();
  node_t* new_node = allocate_node();

  // checking initialization of new_node
  assert(new_node->next == nullptr);
  assert(new_node->status == INVALID);
  assert(new_node->elem == 0);

  new_node->status = ACTIVE;
  new_node->elem = elem;

  // It's okay to add new_node to the tail whose status is OBSOLETE.
  // next_pointer_update() will connect ACTIVE node and new_node.
  node_t* old_tail = __sync_lock_test_and_set(&tail, new_node);
  old_tail->next = new_node;
  __sync_fetch_and_add(&n_size, 1); // increase counter
  __sync_synchronize();
  next_pointer_update();
  __sync_synchronize();

  return new_node;
}

// When two adjacent nodes are concurrently deleted,
// lost update problem can occur.
//
// Make sure that a physically deleted node (added to free list)
// is not on the list for recycling the node in another list of a hash table.
void ConcurrentList::next_pointer_update() {
  node_t *pred = nullptr, *curr = nullptr, *succ = nullptr;

retry:
  while(true) {
    pred = head;
    curr = getNext(pred);
    if(!curr) return;

    while(true) {
      succ = getNext(curr);
      if(!succ) return;

      while(curr->status == OBSOLETE) {
        if(! __sync_bool_compare_and_swap(&pred->next, curr, succ))
          goto retry;

        // new
        // if(!succ) return;

        BVector_flip_and_test(curr->metaData);

        curr = succ;
        succ = getNext(curr);
        if(!succ) return;
      }

      // new
      // if(!succ) return;

      pred = curr;
      curr = succ;
    }
  }
}


// return nullptr if current node is the last

//  If node->next == NULL, then node should be list->tail.
//  If node != list->tail, it means that another Tx is
// updating this lock list.
//  The while loop will end when node->next has value.
ConcurrentList::node_t* ConcurrentList::getNext(node_t *node) {
  while(node->next == nullptr && node != tail) {
    __sync_synchronize();
  }

  return node->next;
}

// Don't worry about race condition.
// Plural threads never get a same node
// because threads can access nodes which they inserted to the list.
void ConcurrentList::erase(node_t *node) {
  node->status = OBSOLETE;
  __sync_synchronize();
  __sync_fetch_and_sub(&n_size, 1); // decrease counter

  // BVector_flip_and_test(node->metaData);
}

void ConcurrentList::reinit_seg_array(int i){
  BVector_turn_on_bits(i);
  for(size_t k = 0; k < IA.s_size; k++) {
    IA.indexArray[i][k].next = nullptr;
    IA.indexArray[i][k].elem = 0;
    IA.indexArray[i][k].status = INVALID;
  }
}

void* ConcurrentList::lazy_deAllocate(void*) {
  if(DBG_DEALLOC)
    printf("deallocator\n");

  while(true) {
    IA.deallocator_sleeping = false;
    printf("deallocator is alive\n");

    next_pointer_update();
    __sync_synchronize();

    // Do not modify head. Stop right before head.
    for(size_t i = 0; i < IA.i_size; i++)
      if(IA.BVector[i] == 0
          && GET_ADR(IA.indexArray[i]) != 0) {

        if(DBG_DEALLOC)
          printf("Reinitializing segment array idx %ld.\n", i);

        // Reinitialize deallocated array
        for(size_t k = 0; k < IA.s_size; k++) {
          IA.indexArray[i][k].next = nullptr;
          IA.indexArray[i][k].elem = 0;
          IA.indexArray[i][k].status = INVALID;
        }
        BVector_turn_on_bits(i);
      }

    if(IA.deallocator_finished == true) {
      break;
    }
    IA.deallocator_sleeping = true;
    usleep(SLEEP_DELAY);
  }


  if(DBG_PREALLOC) {
    printf("finish deallocator\n");
    printf("IndexArray:\n");
    for(int i = 0; i < INDEX_ARRAY_SIZE; i++) {
      printf("%lx\n", (intptr_t)IA.indexArray[i]);
    }
  }

  return nullptr;
}

size_t ConcurrentList::size() {
  return n_size;
}

bool ConcurrentList::empty() {
  return n_size == 0;
}


ConcurrentList::node_t* ConcurrentList::getHead() {
  return head;
}


void* wrap_deallocate(void* arg) {
  ConcurrentList *list = (ConcurrentList*)arg;
  list->lazy_deAllocate(arg);

  return nullptr;
}


void ConcurrentList::initIndexArray() {
  // Calculate BVector size
  int bv_size = 0;
  for(int i = 0; i < LEVEL; i++) {
    bv_size += (0x1 << (3 * i));
  } 
  bv_size *= INDEX_ARRAY_SIZE;

  IA.BVector_size = bv_size;
  IA.BVector = (char*)calloc(IA.BVector_size, sizeof(char));

  // Calculate index array size and segment array size
  IA.i_size = INDEX_ARRAY_SIZE;

  IA.s_size = (size_t) 0x1 << (3 * (LEVEL - 1));

  IA.indexArray = (node_t**)calloc(IA.i_size, sizeof(node_t*));

  for(size_t i = 0; i < IA.i_size; i++) {
    IA.indexArray[i] = (ConcurrentList::node_t*)INVALID_BIT;
  }

  IA.last_used_i_idx = IA.next_s_idx = 0;

  // allocate all arrays.
  for(size_t i = 0; i < IA.i_size; i++) {
    IA.indexArray[i] = allocate_new_array(i);
    BVector_turn_on_bits(i);
  }
  // BVector_turn_on_bits_check();

  // Run deeallocator
  /* IA.deallocator_finished = false;
   * IA.deallocator_sleeping = false;
   * pthread_mutex_init(&IA.deallocator_cond_mutex, nullptr);
   * pthread_cond_init(&IA.deallocator_cond, nullptr);
   * pthread_create(&IA.deAllocator, nullptr, wrap_deallocate, this); */
}

void ConcurrentList::destroyIndexArray() {
  // finish deallocator
/*   pthread_mutex_lock(&IA.deallocator_cond_mutex);
 *   IA.deallocator_finished = true;
 *   pthread_cond_signal(&IA.deallocator_cond);
 *   pthread_mutex_unlock(&IA.deallocator_cond_mutex);
 *
 *   pthread_join(IA.deAllocator, nullptr); */

  // deallocate
  for(int i = 0; i < INDEX_ARRAY_SIZE; i++) {
    node_t* ptr = (node_t*)GET_ADR(IA.indexArray[i]);
    if(ptr != nullptr) {
      free(ptr);
      printf("(destroyIndexArray) Deallocate index array %i.\n", i);
    }
  }

  free(IA.indexArray);

  free(IA.BVector);
}


// ith segment array must be deallocated.
ConcurrentList::node_t* ConcurrentList::allocate_new_array(int i) {
  // Turn on bits in BVector.
  node_t* ptr = (node_t*)calloc(IA.s_size, sizeof(node_t));
  if(!ptr) {
    printf("allocation error\n");
  }

  for(size_t s = 0; s < IA.s_size; s++) {
    ptr[s].metaData.i_idx = i;
    ptr[s].metaData.s_idx = s;
  }
  return ptr;
}


ConcurrentList::node_t* ConcurrentList::allocate_node() {
  size_t i_idx, s_idx, total_s_idx;

  total_s_idx = __sync_fetch_and_add(&IA.next_s_idx, 1);

  // Trying to modulo next_s_idx.
  if(total_s_idx >= IA.i_size * IA.s_size) {
    __sync_bool_compare_and_swap(
        &IA.next_s_idx,
        total_s_idx + 1,
        (total_s_idx + 1) % (IA.i_size * IA.s_size));
  }

  i_idx = (total_s_idx / IA.s_size) % IA.i_size; // % for circular moving
  s_idx = total_s_idx % IA.s_size;

  // Check if the segment array is valid, and the lock is initialized.
  // If both are true, return the lock
  while(true) {
    if(
        // if the segment array is valid
        HAS_INVALID_BIT(IA.indexArray[i_idx]) == false
        // and if the lock is initialized
        && IA.indexArray[i_idx][s_idx].status == INVALID) {

      break;
    } else if(__sync_bool_compare_and_swap(
          &IA.BVector[i_idx], 0, 3)) {
      printf("Reinitializing array %ld.\n", i_idx);
      reinit_seg_array(i_idx);
    }
    next_pointer_update();
    pthread_yield();
  }

  return &IA.indexArray[i_idx][s_idx];
}

void ConcurrentList::BVector_turn_on_bits_recur(int idx) {
  idx++;
  if(idx << 3 < IA.BVector_size) {
    *(long*)(IA.BVector + (idx << 3)) = 0x0101010101010101;
    for(int i = 0; i < 1 << 3; i++) {
      BVector_turn_on_bits_recur((idx << 3) + i);
    }
  }
}

// argument is the idx of index array
void ConcurrentList::BVector_turn_on_bits(int idx) {
  IA.BVector[idx] = 1;

  idx++;
  if(idx << 3 < IA.BVector_size) {
    *(long*)(IA.BVector + (idx << 3)) = 0x0101010101010101;
    for(int i = 0; i < 1 << 3; i++) {
      BVector_turn_on_bits_recur((idx << 3) + i);
    }
  }
}

void ConcurrentList::BVector_flip_and_test(MetaData metaData) {
  int j = LEVEL - 1;
  int i = IA.BVector_size - (0x8 << (3*j)); // start of the level k
  int pos = metaData.i_idx * IA.s_size + metaData.s_idx;

  for(; i >= 0; i -= (0x8 << (3*(--j))) ) {
    IA.BVector[i + pos] = 0x0;
    __sync_synchronize();
    if( * (long *) (IA.BVector + i + (pos&0xFFFFFFF8)) != 0)
      return;
    pos>>=3;
  }
}

// check whether every bits on
void ConcurrentList::BVector_turn_on_bits_check() {
  for(int i = 0; i < IA.BVector_size; i++) {
    assert(IA.BVector[i] == 1);
  }
}

// check whether every bits off
void ConcurrentList::BVector_turn_off_bits_check() {
  for(int i = 0; i < IA.BVector_size; i++) {
    assert(IA.BVector[i] == 0);
  }
}


void ConcurrentList::BVector_turn_off_bits_check_by_array_recur(int idx) {
  idx++;
  if(idx << 3 < IA.BVector_size) {
    assert( *(long*)(IA.BVector + (idx << 3)) == 0);
    for(int i = 0; i < 1 << 3; i++) {
      BVector_turn_off_bits_check_by_array_recur((idx << 3) + i);
    }
  }
}

void ConcurrentList::BVector_turn_off_bits_check_by_array(int idx) {
  assert(IA.BVector[idx] == 0);

  idx++;
  if(idx << 3 < IA.BVector_size) {
    assert( *(long*)(IA.BVector + (idx << 3)) == 0);
    for(int i = 0; i < 1 << 3; i++) {
      BVector_turn_off_bits_check_by_array_recur((idx << 3) + i);
    }
  }
}

