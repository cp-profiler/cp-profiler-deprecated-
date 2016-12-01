namespace cpprofiler {
namespace analysis {


namespace detail {

static void eliminateSubsumedStep(
    NodeTree& nt, std::vector<VisualNode*>& subsumed,
    const std::vector<const ShapeInfo*>& shapes_of_h,
    const std::unordered_map<VisualNode*, ShapeInfo*>& node2shape) {
  for (auto& si : shapes_of_h) {
    for (auto& node : si->nodes) {
      for (auto k = 0u; k < node->getNumberOfChildren(); ++k) {
        auto&& kid = node->getChild(nt.getNA(), k);

        if (node2shape.find(kid) != end(node2shape)) {
          subsumed.push_back(kid);
        }
      }
    }
  }
}

}


/// Filter out unique shapres (with occurrence = 1)
static std::vector<ShapeInfo> filterOutUnique(const std::vector<ShapeInfo>& shapes) {
  std::vector<ShapeInfo> filtered_shapes;
  for (auto& si : shapes) {
    if (si.nodes.size() != 1) {
      filtered_shapes.push_back(si);
    }
  }

  return filtered_shapes;
}

/// Eliminate based on filter settings (update when filter changes)
static void eliminateSubsumed(NodeTree& nt, std::vector<ShapeInfo>& shapes) {
  
  shapes = filterOutUnique(shapes);
  
  /// sort nodes within each shape:
  for (auto& si : shapes) {
    auto& nodes = si.nodes;
    std::sort(begin(nodes), end(nodes));
  }

  /// shapes of height X
  auto maxDepth = nt.getStatistics().maxDepth;

  std::vector<std::vector<const ShapeInfo*>> soh(maxDepth);

  for (auto& shape : shapes) {
    auto h = shape.height;
    soh[h].push_back(&shape);
  }

  std::vector<VisualNode*> subsumed;

  {
      /// NOTE: assuming here that the shapes vector won't change
      /// node2shape should (and will) be destroyed after `eliminateSubsumedStep`
    std::unordered_map<VisualNode*, ShapeInfo*> node2shape;
    for (auto& si : shapes) {
      for (auto& n : si.nodes) {
        node2shape[n] = &si;
      }
    }

    for (int h = 2; h < soh.size(); ++h) {
      detail::eliminateSubsumedStep(nt, subsumed, soh[h], node2shape);
    }
  }

  qDebug() << "subsumed nodes: " << subsumed.size();

  /// remove subsumed from shapes

  std::sort(begin(subsumed), end(subsumed));

  VisualNode dummy_node; /// delete all pointers pointing to this node

  for (auto& si : shapes) {
    std::vector<VisualNode*> new_vec;
    for (auto& n : si.nodes) {

      auto it = std::lower_bound(begin(subsumed), end(subsumed), n);

      if ((it != end(subsumed)) && (*it == n)) {
        n = &dummy_node;
      }
    }

    for (auto n : si.nodes) {
      if (n != &dummy_node) {
        new_vec.push_back(n);
      }
    }

    if (si.nodes.size() != new_vec.size()) {
      si.nodes = std::move(new_vec);
    }
  }

  /// ****************************

  /// some shapes may have become unique
  shapes = filterOutUnique(shapes);
  
}

}}