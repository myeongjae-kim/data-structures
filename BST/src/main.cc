#include <iostream>

#include "bst.h"

int main(void)
{
  int num_elem;
  std::cin >> num_elem;

  bst_t tree;

  for (int i = 0; i < num_elem; ++i) {
    int i_buf;
    std::cin >> i_buf;
    tree.add(i_buf);
    tree.print_inorder();
  }

  std::cin >> num_elem;

  for (int i = 0; i < num_elem; ++i) {
    int i_buf;
    std::cin >> i_buf;
    tree.remove(i_buf);
    tree.print_inorder();
  }

  
  return 0;
}
