/* Binary search tree.
 * Iterative implementation.
 * Type is integer. Not unique elements. */

/* Input
7
2 5 4 6 8 1 3

Output
1 2 3 4 5 6 8
*/

#include <cassert>

class bst_t
{
public:
  bst_t ();
  virtual ~bst_t ();

  bool add(const int);
  bool has(const int);
  bool remove(const int);
  void print_inorder();

private:
  /* data */
  typedef struct node {
    int elem;
    struct node* left;
    struct node* right;
  } node_t;

  node_t* root;

  void destroy_tree(node_t*);

  // These methods return private type.
  // So the definitions of them should be in the class declaration.
  
  node_t* add(node_t* node, const int elem) {
    // Base case
    if (!node) {
      node = new node_t();
      node->elem = elem;
      return node;
    }

    // Recursion
    if (elem <= node->elem) {
      node->left = add(node->left, elem);
    } else {
      node->right = add(node->right, elem);
    }
    return node;
  }

  void print_inorder(node_t* node) {
    if (node) {
      print_inorder(node->left);
      std::cout << node->elem << ' ';
      print_inorder(node->right);
    }
  }

  int get_max_elem_of_subtree(node_t* node) {
    assert(node);
    while (node->right) {
      node = node->right;
    }
    return node->elem;
  }

  node_t* remove(node_t* node, const int target) {
    assert(node);

    // base case
    if (node->elem == target) {
      // both children
      node_t* to_be_deleted = nullptr;
      if (node->left && node->right) {
        int new_target = get_max_elem_of_subtree(node->left);
        node->elem = new_target;
        node->left = remove(node->left, new_target);

        // left child
      } else if (node->left && !node->right){
        to_be_deleted = node;
        node = node->left;
        delete to_be_deleted;

        // right child
      } else if (!node->left && node->right) {
        to_be_deleted = node;
        node = node->right;
        delete to_be_deleted;

        // no children
      } else {
        delete node;
        node = nullptr;
      }

      return node;

      // recursion cases
    } else if (target < node->elem) {
      node->left = remove(node->left, target);
    } else {
      node->right = remove(node->right, target);
    }
    return node;
  }
};
