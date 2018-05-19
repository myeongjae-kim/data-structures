/*
Input
1
5
5
0 3
0 4
2 1
2 3
4 1
*/


#include "graph.h"

#include <vector>

void BFS(graph &g, const int32_t start_node) {
  enum node_status_t {UNVISITED, VISITING, VISITED};
  std::vector<enum node_status_t> nodes_status(g.get_num_nodes());

  std::deque<int32_t> queue;
  int32_t current_node = start_node;

  do {
    nodes_status[current_node] = VISITING;

    std::cout << "VISITING node: " << current_node << std::endl;

    auto &&adj_nodes = g.get_adj_nodes(current_node);

    for (auto i : adj_nodes) {
      if (nodes_status[i] == UNVISITED) {
        queue.push_back(i);
      }
    }

    nodes_status[current_node] = VISITED;

    if (queue.empty()) {
      break;
    } else {
      current_node = queue.front();
      queue.pop_front();
    }
  } while (1);
}

int main(void)
{
  int tc;
  std::cin >> tc;

  for (int t = 1; t <= tc; ++t) {
    int num_node;
    int num_edge;

    std::cin >> num_node >> num_edge;

    graph g(num_node);

    for (int i = 0; i < num_edge; ++i) {
      int from;
      int to;
      std::cin >> from >> to;

      g.add_edge(from, to);
    }

    /* std::cout << "(0, 3): " << g.has_edge(0,3) << std::endl;
     * std::cout << "(0, 4): " << g.has_edge(0,4) << std::endl;
     * std::cout << "(2, 1): " << g.has_edge(2,1) << std::endl;
     * std::cout << "(2, 3): " << g.has_edge(2,3) << std::endl;
     * std::cout << "(4, 1): " << g.has_edge(4,1) << std::endl;
     * std::cout << "(2, 4): " << g.has_edge(2,4) << std::endl; */

    BFS(g, 0);
  }

  return 0;
}
