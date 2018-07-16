#pragma once

#include "ConcurrentList.h"

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
// #define GET_ADR(ptr) (((intptr_t)(ptr) << 16 >>) 16)

class IndexArray
{
public:
  IndexArray (size_t );
  virtual ~IndexArray ();

  ConcurrentList::node_t* allocate_node();

private:
  void allocate_new_array();

  /* data */
  size_t i_size;  // Size of index array.
  size_t s_size;  // Size of segment array.

  size_t head;
  size_t tail;
  size_t last_used_i_idx;
  size_t next_s_idx;
  
  ConcurrentList::node_t** indexArray;
  char* BVector;
};
