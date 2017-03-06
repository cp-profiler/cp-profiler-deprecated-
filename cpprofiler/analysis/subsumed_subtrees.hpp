namespace cpprofiler {
namespace analysis {

using std::vector;
using Group = vector<VisualNode*>;

namespace detail {

static void eliminateSubsumedStep(
  const NodeTree& nt,
  vector<Node*>& subsumed,
  const vector<Group*>& groups_of_h,
  const std::unordered_map<Node*, Group*>& node2group) {

  for (auto& group : groups_of_h) {
    for (auto& n : *group) {
      for (auto k = 0u; k < n->getNumberOfChildren(); ++k) {
        auto&& kid = n->getChild(nt.getNA(), k);

        if (node2group.find(kid) != end(node2group)) {
          subsumed.push_back(kid);
        }
      }
    }
  }
}

}

/// Filter out unique shapes (with occurrence = 1)
static vector<Group> filterOutUnique(const vector<Group>& subtrees) {
  vector<Group> filtered_shapes;
  for (auto& group : subtrees) {
    if (group.size() != 1) {
      filtered_shapes.push_back(group);
    }
  }

  return filtered_shapes;
}


static void eliminateSubsumed(NodeTree& nt, vector<Group>& subtrees) {

  /// sort nodes within each group:
  for (auto& vec : subtrees) {
    std::sort(begin(vec), end(vec));
  }

  auto maxDepth = nt.getStatistics().maxDepth;

  vector< vector<Group*> > soh(maxDepth);

  for (auto& group : subtrees) {
    assert(group.size() > 0);
    auto&& n = group[0];
    auto h = tree_utils::calculateDepth(nt, *n);
    soh[h].push_back(&group);
  }

  /// a flat vector of elements to (potentially) remove later
  vector<Node*> subsumed;

  {
    /// link an element to its group
    std::unordered_map<Node*, Group*> node2group;
    for (auto& vec : subtrees) {
        for (auto&& n : vec) {
            node2group[n] = &vec;
        }
    }

    for (int h = 2; h < soh.size(); ++h) {
        detail::eliminateSubsumedStep(nt, subsumed, soh[h], node2group);
    }
  }

  std::cout << "subsumed count: " << subsumed.size() << "\n";

  /// sorting for faster search (`std::lower_bound`)
  std::sort(begin(subsumed), end(subsumed));

  VisualNode dummy; /// delete all pointers to this node

  for (auto&& group : subtrees) {
    Group new_group;
    for (auto&& n : group) {
        auto it = std::lower_bound(begin(subsumed), end(subsumed), n);

        if (it != end(subsumed) && (*it == n)) {
            n = &dummy;
        }
    }

    for (auto&& n : group) {
        if (n != &dummy) {
            new_group.push_back(n);
        }
    }

    if (group.size() != new_group.size()) {
        group = std::move(new_group);
    }
  }

  vector<Group> result;

  /// remove empty groups
  for (auto& g : subtrees) {
    if (g.size() != 0) {
        result.push_back(g);
    }
  }

  subtrees = std::move(result);

}

}}