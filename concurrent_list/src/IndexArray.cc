#include "IndexArray.h"

IndexArray::IndexArray (size_t i_size) {
  if(i_size < 10) i_size = 10; // At least 10 segment arrays

  s_size = (size_t) 0x1 << (3 * (LEVEL - 1));
  
  this->i_size = i_size;

  indexArray = new ConcurrentList::node_t*[i_size];

  for(size_t i = 0; i < i_size; i++) {
    indexArray[i] = (ConcurrentList::node_t*)INVALID_BIT;
  }

  head = tail = last_used_i_idx = next_s_idx = 0;
  
  // allocate five arrays first.
  head += 5;
  for(size_t i = 0; i < head; i++) {
    indexArray[i] = new ConcurrentList::node_t[s_size]();
    for(size_t s = 0; s < s_size; s++) {
      indexArray[i][s].metaData.i_idx = i;
      indexArray[i][s].metaData.s_idx = s;
    }
  }
}

#include <cstdio>
#include <cstdlib>

ConcurrentList::node_t* IndexArray::allocate_node() {
  if(next_s_idx == s_size) { 
     last_used_i_idx = (last_used_i_idx + 1) % i_size;
     next_s_idx = 0;
  }

  // TODO: low-water mark

  if(last_used_i_idx >= head) {
    printf("No more pre-allocated memory\n");
    exit(1);
  }

  return &indexArray[last_used_i_idx][next_s_idx++];
}


IndexArray::~IndexArray () {
  delete[] indexArray;
}
