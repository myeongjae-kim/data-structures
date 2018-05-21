#include <iostream>
#include <vector>

#include "bst.h"

void make_minimal_tree(bst_t &bst, std::vector<int> &v,
                       int idx_start, int num_elem) {
  if (num_elem == 0) {
    return;
  } else if (num_elem == 1) {
    bst.insert(v[idx_start]);
    return;
  }

  int idx_median = (idx_start + (idx_start + num_elem - 1)) / 2;
  bst.insert(v[idx_median]);

  int next_vector_size = (num_elem - 1) / 2;

  make_minimal_tree(bst, v, idx_start, next_vector_size);
  make_minimal_tree(bst, v, idx_median + 1,
      num_elem & 1 ? next_vector_size : next_vector_size + 1);

  return;
}


int main(void)
{
  int tc;
  std::cin >> tc;

  for (int t = 1; t <= tc; ++t) {
    int num_elem;
    std::cin >> num_elem;

    std::vector<int> v;

    for (int i = 0; i < num_elem; ++i) {
      int i_buf;
      std::cin >> i_buf;
      v.push_back(i_buf);
    }

    bst_t bst;
    make_minimal_tree(bst, v, 0, num_elem);

    
    std::cout << "Case #" << t <<": "  << bst.get_height() << std::endl;
    // bst.print_inorder();
  }

  return 0;
}
