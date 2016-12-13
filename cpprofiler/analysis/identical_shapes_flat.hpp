namespace cpprofiler {
namespace analysis {
namespace subtrees {

namespace identical_subtrees_flat {

struct Separator {
  int mark;
  int new_mark;
};

template<typename K, typename T>
static void printMap(const std::unordered_map<K, T>& map) {
  std::stringstream ss;

  for (auto &p : map) {
    ss << p.first << ": " << p.second << " ";
  }

  qDebug() << ss.str().c_str();
}

template<typename T>
static void printVector(const std::vector<T>& v, int start, int end) {
  std::stringstream ss;

  auto it_start = begin(v); std::advance(it_start, start);
  auto it_end   = begin(v); std::advance(it_end, end);

  for (auto it = it_start; it != it_end; ++it) {
    ss << *it << " ";
  }

  qDebug() << ss.str().c_str();
}

template<typename T>
static void printVector(const std::vector<T>& v) {
  printVector(v, 0, v.size());
}

#ifdef MAXIM_DEBUG
static void print_partition(const std::vector<ChildInfo>& subtrees,
                            const std::vector<Separator>& gr_separators)
{

  int sep_idx = 0;
  int next_sep = gr_separators[0].mark;

  int sep_idx2 = 0;
  int next_sep2 = gr_separators[0].new_mark;
  std::stringstream ss;

  printVector(gr_separators);

  for (auto i = 0u; i < subtrees.size(); ++i) {


    if (i == next_sep) {
      ss << " # ";
      ++sep_idx;
      if (sep_idx < gr_separators.size()) {
        next_sep = gr_separators[sep_idx].mark;
      }
    }

    if (i == next_sep2) {
      ss << " | ";
      ++sep_idx2;
      if (sep_idx2 < gr_separators.size()) {
        next_sep2 = gr_separators[sep_idx2].new_mark;
      }
    }

    ss << subtrees[i].node->debug_id << " ";
  }

  qDebug() << ss.str().c_str();
}
#endif


static void printMap(const std::unordered_map<const VisualNode*, PosInGroups>& map,
                      const std::unordered_map<int, int>& id2idx) {
  std::stringstream ss;

  for (auto &p : map) {
    auto pos = p.second;
    #ifdef MAXIM_DEBUG
    ss << p.first->debug_id;
    #else
    ss << (void*)p.first;
    #endif
    ss << ": " << "(" << id2idx.at(pos.g_id) << ", " << pos.inner_idx << ")" << " ";
  }

  qDebug() << ss.str().c_str();
}

static std::stringstream& operator<<(std::stringstream& ss, const Separator& s) {
  ss << "(" << s.mark << "," << s.new_mark << ")";
  return ss;
}

static std::ostream& operator<<(std::ostream& ss, const PosInGroups& p) {
  ss << "(" << p.g_id << "," << p.inner_idx << ")";
  return ss;
}

static std::ostream& operator<<(std::ostream& os, const VisualNode*& n) {
  #ifdef MAXIM_DEBUG
      os << n->debug_id;
  #else
      os << (void*)n;
  #endif
  return os;
}

#ifdef MAXIM_DEBUG
static void printGroup(const Group& g) {
  std::stringstream ss;

  for (auto i = 0u; i < g.splitter; ++i) {
    ss << g.items[i].node->debug_id << " ";
  }

  ss << "|";

  for (auto i = g.splitter; i < g.items.size(); ++i) {
    ss << g.items[i].node->debug_id << " ";
  }
  qDebug() << ss.str().c_str();
}

static void printGroups(const std::vector<Group>& groups) {
  for (const auto& g : groups) {
    printGroup(g);
  }
}

#endif

/// NOTE(maxim): this makes the algorithm non-linear time
static void updateGroups(const std::vector<ChildInfo>& subtrees,
                         std::vector<Separator>& separators,
                         std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
                         std::vector<int>& id2idx) {

  // qDebug() << "before update:";
  // printVector(separators);

  
  for (auto i = 0u; i < separators.size(); ++i) {

    auto& s = separators[i];
    /// nothing moved
    if (s.new_mark == s.mark) continue;

    /// figure out last pos
    int last_pos;

    if (i == separators.size() -1) {
      /// corner case: last separator
      last_pos = subtrees.size();
    } else {
      last_pos = separators[i+1].mark;
    }

    /// all moved -> reset
    if (s.new_mark == last_pos) {
      s.new_mark = s.mark;
      continue;
    }


    /// create a new spearator based on new_mark
    Separator new_sep({s.new_mark, s.new_mark});

    /// reset the current group
    s.new_mark = s.mark;

    /// insert the new group right after
    auto new_group_idx = i + 1;


    auto it = begin(separators); std::advance(it, new_group_idx);

    separators.insert(it, new_sep); // 105ms

    // perfHelper.accumulate("update1"); // 4s
 
    // id of the new (extracted) group
    id2idx.push_back(new_group_idx);
    auto group_id = id2idx.size() - 1;

    /// TODO(maxim): can this be taken in the outer loop?
    for (auto i = 0u; i < id2idx.size() - 1; ++i) {
      if (id2idx[i] >= new_group_idx) {
        id2idx[i] += 1;
      }
    }
    // perfHelper.end("update1");
    // perfHelper.accumulate("update2"); // 2s

    /// all nodes from new_sep to last_pos should be remapped
    for (auto j = new_sep.mark; j < last_pos; ++j) {
      const auto& n = subtrees[j];
      auto& pos = node2groupID[n.node];

      pos.g_id = group_id;
      pos.inner_idx = j - new_sep.mark;      
    }

    // perfHelper.end("update2");

  }

}

static PosInGroups findNode(
    const std::unordered_map<const VisualNode*, PosInGroups>& node2groupID,
    const std::vector<int>& id2idx,
    const VisualNode* node) {

  // qDebug() << "find node: " << node->debug_id;
  
  const auto pos = node2groupID.at(node);
  const auto g_id = pos.g_id;
  const auto g_idx = id2idx.at(g_id);
  const auto i_idx = pos.inner_idx;

  return {g_idx, i_idx};
}

/// TODO(maxim): can I make it linear time? (not used currently)
GroupsOfNodes_t findIdentical(NodeTree& nt, const GroupsOfNodes_t& init_p) {

  /// ------ 0) Initial Partition ------
  std::vector<Group> groups = prepareGroups(init_p, nt);

  /// ------ 1) assign a group id to each node -------
  std::unordered_map<const VisualNode*, PosInGroups> node2groupID;
  // std::unordered_map<int, int> id2idx;
  std::vector<int> id2idx;

  for (auto g_idx = 0u; g_idx < groups.size(); ++g_idx) {
    auto& group = groups[g_idx];

    for (auto i = 0u; i < group.items.size(); ++i) {
      auto* node = group.items[i].node;
      node2groupID[node].g_id = g_idx;
      node2groupID[node].inner_idx = i;
    }

    id2idx.push_back(g_idx);
  }


  /// ------ 2) construct a flat vector of ChildInfo ---------
  std::vector<ChildInfo> subtrees;
  std::vector<Separator> group_separators = { {0, 0} }; // put first separator at 0

  int count = 0;
  for (auto& g : groups) {
    for (auto& ci : g.items) {
      subtrees.push_back(ci); // copy here
    }
    count += g.items.size();
    group_separators.push_back({count, count});
  }


  // print_partition(subtrees, group_separators);

  /// ---- 3) do stuff for each group in order (starting with the most shallow) ----
  // for (auto g_idx = 0; g_idx < 1; ++g_idx) {
  for (auto g_idx = 0; g_idx < group_separators.size() - 1; ++g_idx) {

    int g_beg = group_separators[g_idx].mark;
    int g_end = group_separators[g_idx + 1].mark;
    
    /// do stuff for left and then right children
    /// NOTE(maxim): only works with binary trees for now
    for (auto alt = 0u; alt < 2; ++alt) {

      for (auto i = g_beg; i < g_end; ++i) {
        auto& ci = subtrees[i];

        if (ci.alt != alt) continue;

        /// 3.1) get node's parent
        const auto& parent = ci.node->getParent(nt.getNA());

        /// 3.2) find the parent in groups

        const auto pos = findNode(node2groupID, id2idx, parent);
        auto g_idx = pos.g_id;
        auto idx = pos.inner_idx;

        /// 3.3) separate the node from the rest of its group

        /// add new separator if needed
        /// swap parent with the element right after the new separator
        auto  p_pos = group_separators[g_idx].mark + idx;
        auto& n_pos = group_separators[g_idx].new_mark;

        auto& el_1 = subtrees[p_pos];
        auto& el_2 = subtrees[n_pos];
        
        /// keep the mapping updated

        std::swap(node2groupID[el_1.node].inner_idx, node2groupID[el_2.node].inner_idx);
        std::swap(el_1, el_2);
        ++n_pos;
    
      }

      updateGroups(subtrees, group_separators, node2groupID, id2idx);

    }

  }

  perfHelper.total("map access");
  perfHelper.total("rest");


  std::vector<std::vector<VisualNode*>> result;
  result.reserve(group_separators.size());

  /// construct the result

  // qDebug() << "constructing the result";
  for (auto i = 0u; i < group_separators.size() - 1; ++i) {
    auto it_begin = begin(subtrees); std::advance(it_begin, group_separators[i].mark);
    auto it_end = begin(subtrees); std::advance(it_end, group_separators[i + 1].mark);

    std::vector<VisualNode*> group;
    for (auto it = it_begin; it < it_end; ++it) {
      group.push_back(it->node);
      // qDebug() << "node: " << it->node->debug_id;
    }

    // qDebug() << "-----------";

    result.push_back(group);
  }

  return result;
}

}}}}