namespace cpprofiler {
namespace analysis {

using std::vector;
using Group = vector<VisualNode*>;

namespace detail {

/// Go from shallowest groups to the deepest and see if immediate children
/// of corresponding nodes are roots for other groups of subtrees
static void eliminateSubsumedStep(
    const NodeTree& nt, Group& subsumed, const vector<Group*>& groups_of_h,
    const std::unordered_map<VisualNode*, Group*>& node2group) {
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


static void eliminateSubsumed(const NodeTree& nt, vector<Group>& subtrees) {

  /// NOTE(maxim): it is important that `subtrees` doesn't grow in size,
  /// because pointers to `Group` are used

  /// Sort nodes within each group: (why?)
  for (auto& vec : subtrees) {
    std::sort(begin(vec), end(vec));
  }

  /// Work out subtree depths and populate soh ("subtrees of height")
  const auto maxDepth = nt.getStatistics().maxDepth;
  vector< vector<Group*> > soh(maxDepth);

  for (auto& group : subtrees) {
    auto node = group[0];
    auto h = tree_utils::calculateDepth(nt, *node);
    soh[h].push_back(&group);
  }

  /// a flat vector of elements to (potentially) remove later
  vector<VisualNode*> subsumed;

  {
    /// Remember which group a node belongs to
    std::unordered_map<VisualNode*, Group*> node2group;
    for (auto& group : subtrees) {
        for (auto node : group) {
            node2group[node] = &group;
        }
    }

    /// Populate `subsumed`
    for (int h = 2; h < soh.size(); ++h) {
        detail::eliminateSubsumedStep(nt, subsumed, soh[h], node2group);
    }
  }

  std::cout << "subsumed count: " << subsumed.size() << "\n";

  /// sorting for faster search (`std::lower_bound`)
  std::sort(begin(subsumed), end(subsumed));

  VisualNode dummy; /// delete all pointers to this node

  vector<Group> result;

  /// R-value reference to be able to move into?
  /// TODO(maxim): remove only if the whole group is subsumed
  for (auto&& group : subtrees) {

    Group tmp_group = group;
    for (auto&& n : tmp_group) {
        auto it = std::lower_bound(begin(subsumed), end(subsumed), n);

        if (it != end(subsumed) && (*it == n)) {
            n = &dummy;
        }
    }

    /// count nodes marked as dummy
    auto total_in_group = group.size();
    auto dummy_count = 0u;

    for (auto n : tmp_group) {
        if (n == &dummy) {
            dummy_count++;
        }
    }

    /// only keep groups not fully subsumed
    if (dummy_count != total_in_group) {
      result.push_back(group);
    }
  }

  subtrees = std::move(result);

}

}}