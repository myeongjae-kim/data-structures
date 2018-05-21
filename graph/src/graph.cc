#include "graph.h"

graph::graph(const int32_t num_nodes) {
  this->num_nodes = num_nodes;
#ifdef ADJACENCY_LIST
  adj_list.resize(num_nodes);
#else
  adj_mat.resize(num_nodes);
  for (auto& i : adj_mat) {
    i.resize(num_nodes);
  }
#endif
}

bool graph::is_valid_node(const int32_t node_name) {
#ifdef ADJACENCY_LIST
  if (node_name >= (int32_t)adj_list.size()){
#else
  if (node_name >= (int32_t)adj_mat.size()) {
#endif
    std::cerr << "(is_valid_node) node name " << node_name
      << " is bigger than last node" << std::endl;
    return false;
  } else {
    return true;
  }
}

bool graph::add_edge(const int32_t from, const int32_t to){
  if ( ! (is_valid_node(from) && is_valid_node(to)) ) {
    return false;
  }
#ifdef ADJACENCY_LIST
  adj_list[from].push_back(to);
  adj_list[to].push_back(from);
#else
  adj_mat[from][to] = true;
  adj_mat[to][from] = true;
#endif
  return true;
}

bool graph::del_edge(const int32_t from, const int32_t to){
#ifdef ADJACENCY_LIST
  auto it = adj_list[from].begin();
  for (;
        it != adj_list[from].end(); ++it) {
    if (*it == to) {
      adj_list[from].erase(it);
      break;
    }
  }

  if (it == adj_list[from].end()) {
    std::cerr << "(del_node) no edge ( " << from << ", " << to
      << ") to be deleted" << std::endl;
    return false;
  }

  for (it = adj_list[to].begin(); it != adj_list[to].end(); ++it) {
    if (*it == from) {
      adj_list[to].erase(it);
      break;
    }
  }

  if (it == adj_list[to].end()) {
    std::cerr << "(del_node) no edge ( " << to << ", " << from
      << ") to be deleted" << std::endl;
    return false;
  }
#else
  if (adj_mat[from][to] == false || adj_mat[to][from] == false) {
    std::cerr << "(del_node) no edge to be deleted" << std::endl;
    return false;
  } else {
    adj_mat[from][to] = false;
    adj_mat[to][from] = false;
  }
#endif

  return true;
}

bool graph::has_edge(const int32_t from, const int32_t to){
#ifdef ADJACENCY_LIST
  for (auto it = adj_list[from].begin();
        it != adj_list[from].end();
        ++it) {
    if (*it == to) {
      return true;
    }
  }
  return false;
#else
  return adj_mat[from][to];
#endif
}

std::deque<int32_t> graph::get_adj_nodes(const int32_t node) {
#ifdef ADJACENCY_LIST
  return adj_list[node];
#else
  std::deque<int32_t> adj_nodes;

  std::vector<bool>& to_nodes = adj_mat[node];

  for (int i = 0; i < to_nodes.size(); ++i) {
    if (to_nodes[i] == true) {
      adj_nodes.push_back(i);
    }
  }
  return adj_nodes;
#endif
}

int32_t graph::get_num_nodes() {
  return num_nodes;
}

graph::~graph() {

}
