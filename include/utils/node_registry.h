#pragma once

#include <algorithm>
#include <optional>
#include <pipewire/client.h>
#include <vector>

namespace utils {
struct Node {
  unsigned int id;
  pw_client *client;
};

class NodeRegistry {
public:
  void add_node(unsigned int id, pw_client *client) {
    nodes.push_back({id, client});
  }

  void delete_node_by_id(unsigned int id) {
    auto position = std::find_if(nodes.begin(), nodes.end(),
                                 [id](auto node) { return node.id == id; });
    if (position != nodes.end()) {
      nodes.erase(position);
    }
  }

  std::optional<Node> get_node_by_id(unsigned int id) const {
    for (auto node : nodes) {
      if (node.id == id) {
        return node;
      }
    }
    return std::nullopt;
  }

private:
  std::vector<Node> nodes;
};
} // namespace utils
