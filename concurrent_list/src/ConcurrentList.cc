#include "ConcurrentList.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#define DBG_PREALLOC true
#define DBG_DEALLOC true


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

        curr = succ;
        succ = getNext(curr);
        if(!succ) return;
      }
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

  BVector_flip_and_test(node->metaData);
}

void* ConcurrentList::lazy_deAllocate(void*) {
  if(DBG_DEALLOC)
    printf("deallocator\n");

  while(!IA.deallocator_finished) {
    IA.deallocator_sleeping = false;

    next_pointer_update();
    __sync_synchronize();

    for(size_t i = IA.tail;
        i != IA.head;
        i++) {
      i %= IA.i_size;

      if(DBG_DEALLOC)
        printf("Trying to deallocate segment array idx %ld.\n", i);

      if(IA.BVector[i] == 0
          && (node_t*)GET_ADR(IA.indexArray[i]) != nullptr) {
        // TODO: Check safety condition of Transactions
        // If safety condition is satisfied, deallocate the segment array


        if(DBG_DEALLOC)
          printf("Deallocate segment array idx %ld.\n", i);

        IA.indexArray[i] = (node_t*)SET_INVALID_BIT(IA.indexArray[i]);
        free((void*)GET_ADR(IA.indexArray[i]));
        IA.indexArray[i] = (node_t*)SET_INVALID_BIT(nullptr);

        // update tail
        for(size_t k = 0;
            k < IA.i_size && GET_ADR(IA.indexArray[IA.tail]) == 0;
            k++) {
          IA.tail++;
          IA.tail %= IA.i_size;
        }
      } else {
        if(DBG_DEALLOC)
          printf("Fail to deallocate segment array idx %ld.\n", i);
      }
    }
    

    pthread_mutex_lock(&IA.deallocator_cond_mutex);
    IA.deallocator_sleeping = true;
    if(IA.deallocator_finished == true) break;

    if(DBG_DEALLOC)
      printf("sleep deallocator\n");
    // pthread_cond_wait(&IA.deallocator_cond, &IA.deallocator_cond_mutex);
    usleep(200000);

    pthread_mutex_unlock(&IA.deallocator_cond_mutex);
  }


  if(DBG_PREALLOC) {
    printf("finish deallocator\n");
    printf("IA.tail: %ld\n", IA.tail);
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



void* wrap_preallocate(void* arg) {
  ConcurrentList *list = (ConcurrentList*)arg;
  list->preAllocate(arg);

  return nullptr;
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

  IA.head = IA.tail = IA.last_used_i_idx = IA.next_s_idx = 0;


  // allocate five arrays first.
  // IA.head = (largest allocated offset + 1)
  IA.head += 2;
  for(size_t i = 0; i < IA.head; i++) {
    allocate_new_array(i);
  }
  // BVector_turn_on_bits_check();

  // Run preallocator
  IA.preallocator_finished = false;
  IA.preallocator_sleeping = false;
  pthread_mutex_init(&IA.preallocator_cond_mutex, nullptr);
  pthread_cond_init(&IA.preallocator_cond, nullptr);
  pthread_create(&IA.preAllocator, nullptr, wrap_preallocate, this);

  // Run preallocator
  IA.deallocator_finished = false;
  IA.deallocator_sleeping = false;
  pthread_mutex_init(&IA.deallocator_cond_mutex, nullptr);
  pthread_cond_init(&IA.deallocator_cond, nullptr);
  pthread_create(&IA.deAllocator, nullptr, wrap_deallocate, this);
}

void ConcurrentList::destroyIndexArray() {
  // finish preallocator
  pthread_mutex_lock(&IA.preallocator_cond_mutex);
  IA.preallocator_finished = true;
  pthread_cond_signal(&IA.preallocator_cond);
  pthread_mutex_unlock(&IA.preallocator_cond_mutex);

  pthread_join(IA.preAllocator, nullptr);


  // finish deallocator
  pthread_mutex_lock(&IA.deallocator_cond_mutex);
  IA.deallocator_finished = true;
  pthread_cond_signal(&IA.deallocator_cond);
  pthread_mutex_unlock(&IA.deallocator_cond_mutex);

  pthread_join(IA.deAllocator, nullptr);

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


void ConcurrentList::allocate_new_array(int i) {
  // Turn on bits in BVector.
  BVector_turn_on_bits(i);

  if((IA.indexArray[i] = (node_t*)calloc(IA.s_size, sizeof(node_t)))
      == nullptr) {
    printf("Allocation error\n");
    assert(false);
  }

  for(size_t s = 0; s < IA.s_size; s++) {
    IA.indexArray[i][s].metaData.i_idx = i;
    IA.indexArray[i][s].metaData.s_idx = s;
  }
}


bool ConcurrentList::is_new_allocation_needed() {
  size_t i_idx = IA.next_s_idx / IA.s_size;

  return abs(i_idx - IA.head) <= 1;
}

ConcurrentList::node_t* ConcurrentList::allocate_node() {
  size_t i_idx, s_idx, total_s_idx;

  total_s_idx = __sync_fetch_and_add(&IA.next_s_idx, 1);

  i_idx = (total_s_idx / IA.s_size) % IA.i_size; // % for circular moving
  s_idx = total_s_idx % IA.s_size;

  // Wake up preallocator it it needs to be called.
  if(is_new_allocation_needed()) {
    bool wait_preallocator = false;

    pthread_mutex_lock(&IA.preallocator_cond_mutex);
    IA.preallocator_sleeping = false;

    // Do I have to wait preallocator?
    wait_preallocator = HAS_INVALID_BIT(IA.indexArray[i_idx]);
    pthread_cond_signal(&IA.preallocator_cond);
    pthread_mutex_unlock(&IA.preallocator_cond_mutex);

    // pause if there is no more allocated memory
    // IA.head is modified in the preallocator and additional memory is allocated.
    // Wait until the preallocator is done
    //
    // If curent i_idx is s_idx is in alreay allocated segment array,
    // do not wait preallocator.

    /* while(total_s_idx >= (size_t)(IA.head * IA.s_size)
     *     || IA.preallocator_sleeping == false); */
    if(wait_preallocator)
      while(IA.preallocator_sleeping == false);
  }

  return &IA.indexArray[i_idx][s_idx];
}

// Thread main
void* ConcurrentList::preAllocate(void*) {
  if(DBG_PREALLOC)
    printf("preallocator\n");

  while(!IA.preallocator_finished) {
    // If there is no free slot of segment array,
    // Exit with error message

    // allocate additional segment array and move head foward
    if(is_new_allocation_needed()) {
      if(DBG_PREALLOC)
        printf("Trying to allocate new array by preallocator\n");

      // TODO: Make it circular
      size_t old_head = __sync_fetch_and_add(&IA.head, 1);

      if(old_head < IA.i_size) {
        allocate_new_array(old_head);

        if(DBG_PREALLOC)
          printf("New array is allocated.\n");
      } else if(old_head == IA.i_size){
        if(DBG_PREALLOC) {
          __sync_fetch_and_sub(&IA.head, 1);
          printf("Fail to allocate new array.");
          printf("Now only one array is left.\n");

          printf("IndexArray:\n");
          for(int i = 0; i < INDEX_ARRAY_SIZE; i++) {
            printf("%lx\n", (intptr_t)IA.indexArray[i]);
          }

        }
      } else {
        printf("No more segment array slot.\n");
        printf("Increase the semgnet array number.\n");
        exit(1);
      }
    }


/*     if(IA.head >= IA.last_used_i_idx
 *         && IA.head - IA.last_used_i_idx < 3) {
 *       // TODO
 *
 *     } else if(IA.head < IA.last_used_i_idx
 *         && (IA.head + IA.i_size) - IA.last_used_i_idx < 3) {
 *       // TODO
 *
 *     } */

    pthread_mutex_lock(&IA.preallocator_cond_mutex);
    IA.preallocator_sleeping = true;
    if(IA.preallocator_finished == true) {
      pthread_mutex_unlock(&IA.preallocator_cond_mutex);
      break;
    }

    if(DBG_PREALLOC)
      printf("sleep preallocator\n");
    pthread_cond_wait(&IA.preallocator_cond, &IA.preallocator_cond_mutex);
    pthread_mutex_unlock(&IA.preallocator_cond_mutex);
  }

  if(DBG_PREALLOC)
    printf("finish preallocator\n");

  return nullptr;
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

