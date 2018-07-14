#include "ConcurrentList.h"

// Where is the location of OBSOLETE?
// A variable, or a bit of next pointer?
// If the second option is correct, we must have pred and curr node
// to mark the OBSOLETE bit.

ConcurrentList::ConcurrentList() {
  n_size = 0;
  head = new node_t();

  // checking initialization of new_node
  assert(head->next == nullptr);
  assert(head->status == INVALID);
  assert(head->elem == 0);

  head->status = HEAD; // Dummy head

  tail = head;
}

ConcurrentList::~ConcurrentList() {
  while(head) {
    node_t* next = head->next;
    delete head;
    head = next;
  }
}

// Insert a node to a list.
// First, update tail node using atomic instruction.
// Second, connect old_tail to new tail.
// Third, call next_pointer_update() to skip OBSOLETE nodes.
ConcurrentList::node_t* ConcurrentList::push_back(int elem) {
  node_t* new_node = new node_t();

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

    while(true) {
      if(!curr) return;

      succ = getNext(curr);
      if(!succ) return;

      while(curr->status == OBSOLETE) {
        if(! __sync_bool_compare_and_swap(&pred->next, curr, succ))
          goto retry;

        curr = succ;
        if(!curr) return;

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
}

void ConcurrentList::deallocate(node_t *node) {

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
