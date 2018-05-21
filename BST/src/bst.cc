#include <iostream>
#include <climits>
#include "bst.h"

bst_t::bst_t () {
  root = nullptr;
}

bst_t::~bst_t () {
  destroy_tree(root);
  // root = nullptr;
}

void bst_t::destroy_tree(node_t* node) {
  if (node) {
    destroy_tree(node->left);
    destroy_tree(node->right);
    delete node;
  }
}

bool bst_t::add (int elem) {
  root = add(root, elem);
  return true;
}

bool bst_t::has (int elem) {
  node_t* iter = root;

  while (iter) {
    if (elem < iter->elem) {
      iter = iter->left;
    } else if(elem > iter->elem) {
      iter = iter->right;
    } else {
      return true;
    }
  }

  return false;
}

bool bst_t::remove(const int elem) {
  if (has(elem) == false) {
    return false;
  }

  root = remove(root, elem);

  return true;
}

void bst_t::print_inorder() {

  if (!root) {
    std::cout << "Tree is empty" << std::endl;
    return;
  }

  print_inorder(root);
  std::cout << std::endl;
}
