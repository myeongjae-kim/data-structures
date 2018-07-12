// This linked list is based on the paper:
//  A Scalable Lock Manager for Multicores, Hyungsoo Jung, et al.

#include <cstddef>
#include <cassert>
#include <cstdint>

class ConcurrentList
{
public:
  // Types definition
  enum status {INVALID, HEAD, ACTIVE, WAIT, OBSOLETE};
  struct node {
    struct node* next;
    enum status status;
    int elem;
  };
  typedef struct node node_t;


  // Constructor and destructor
  ConcurrentList ();
  virtual ~ConcurrentList ();

  // Methods
  // Insert a value
  void insert(int);

  // Update next pointer of all nodes in the list
  void next_pointer_update();

  // Lazy deletion. This method just changes a node's status to OBSOLETE
  void erase(node_t*);

  // This method is used for safe iteration of list.
  node_t* getNext(node_t*);

  // return head
  node_t* getHead();

  // Deallocate OBSOLETEd nodes
  void deallocate(node_t*);

  size_t size();
  bool empty();

private:
  /* data */
  size_t n_size;

  node_t* head;
  node_t* tail;
};
