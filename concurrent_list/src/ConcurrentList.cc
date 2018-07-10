#include "ConcurrentList.h"

ConcurrentList::ConcurrentList() {
  n_size = 0;
  head = new node_t();

  // checking initialization of new_node
  assert(head->next == nullptr);
  assert(head->status == ACTIVE);
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
void ConcurrentList::insert(int elem) {
  node_t* new_node = new node_t();

  // checking initialization of new_node
  assert(new_node->next == nullptr);
  assert(new_node->status == ACTIVE);
  assert(new_node->elem == 0);

  new_node->status = ACTIVE;
  new_node->elem = elem;

  node_t* old_tail = __sync_fetch_and_add(&tail, new_node);
  old_tail->next = new_node;
  __sync_fetch_and_add(&n_size, 1); // increase counter
  __sync_synchronize();
  next_pointer_update();
  __sync_synchronize();
}

void ConcurrentList::next_pointer_update() {
  // TODO
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
// Plural threads never get a same node.
void ConcurrentList::erase(node_t *node) {
  node->status = OBSOLETE;
  __sync_fetch_and_sub(&n_size, 1); // increase counter
}

void ConcurrentList::deallocate(node_t *node) {

}

size_t ConcurrentList::size() {
  return n_size;
}

bool ConcurrentList::empty() {
  return n_size == 0;
}
