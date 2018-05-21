#include <vector>
#include <deque>
#include <cstdint>
#include <iostream>

/* By commenting below #define,
 * We can change implementation from adjacency list to adjacecny matrix
 * TODO: implementing adjacency hash */
#define ADJACENCY_LIST

/* The graph is undirected, unweight graph */

/* Node names are integer.
 * If num_nodes is 10, then nodes are 0~9 */

class graph
{
public:
  graph (const int32_t num_nodes);
  bool add_edge(const int32_t from, const int32_t to);
  bool del_edge(const int32_t from, const int32_t to);
  bool has_edge(const int32_t from, const int32_t to);
  std::deque<int32_t> get_adj_nodes(const int32_t node);
  virtual ~graph ();

  int32_t get_num_nodes();

private:
  bool is_valid_node(const int32_t);
  int32_t num_nodes;

#ifdef ADJACENCY_LIST
  std::vector< std::deque<int32_t> > adj_list;
#else
  std::vector< std::vector<bool> > adj_mat;
#endif
};
