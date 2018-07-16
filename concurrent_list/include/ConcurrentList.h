// This linked list is based on the paper:
//  A Scalable Lock Manager for Multicores, Hyungsoo Jung, et al.
#pragma once

#include <cstddef>
#include <cassert>
#include <cstdint>

/* #define OBSOLETE (1ULL << 63)
 * #define IS_OBSOLETE(ptr) (((intptr_t)(ptr) & OBSOLETE) != 0)
 * #define SET_OBSOLETE(ptr) ((intptr_t)(ptr) |= OBSOLETE) */

/****** These are for index array ******/
#define LEVEL (4) // Level of segment array
// s_size = (size_t) 0x1 << (3 * (LEVEL -1));
// Level 4:
//        1
//        8
//       64
//       512
//
// Each segment array has 512 locks.
// If a size of IndexArray is 8, then this array has 512 * 8 = 4096 locks.

#define INVALID_BIT (1ULL<<63);
#define SET_INVALID_BIT(ptr) ((intptr_t)(ptr) | INVALID_BIT)
#define RESET_INVALID_BIT(ptr) ((intptr_t)(ptr) & ~INVALID_BIT)
#define GET_ADR(ptr) (((intptr_t)(ptr) << 16 >>) 16)




class ConcurrentList
{
public:
  // Types definition
  enum status {INVALID, HEAD, ACTIVE, WAIT, OBSOLETE};

  typedef struct MetaData {
    int i_idx; // index array idx
    int s_idx; // segment array idx
  } MetaData;

  struct node {
    struct node* next;
    enum status status;
    int elem;
    MetaData metaData;
  };
  typedef struct node node_t;


  // Constructor and destructor
  ConcurrentList ();
  virtual ~ConcurrentList ();

  // Methods
  // Insert a value
  node_t* push_back(int);

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

  // Below is for IndexArray
  typedef struct IndexArray {
      /* data */
    size_t i_size;  // Size of index array.
    size_t s_size;  // Size of segment array.

    size_t head;
    size_t tail;
    size_t last_used_i_idx;
    size_t next_s_idx;
    
    ConcurrentList::node_t** indexArray;
    char* BVector;
  } IndexArray;

  IndexArray IA;

  void initIndexArray(size_t);
  void destroyIndexArray();

  void allocate_new_array();

  ConcurrentList::node_t* allocate_node();

};
