#include "similar_shape_algorithm.hh"
#include <set>
#include <stack>
#include <memory>

#include "visualnode.hh"
#include "nodetree.hh"
#include "cpprofiler/utils/tree_utils.hh"

namespace cpprofiler {
namespace analysis {

namespace detail {

/// for temporary data structure
struct ShapeI {
  int sol;
  int shape_size;
  int shape_height;
  VisualNode* node;
  Shape* s;
  ShapeI(int sol0, VisualNode* node0)
      : sol(sol0), node(node0), s(Shape::copy(node->getShape())) {
    shape_size = shapeSize(*s);
    shape_height = s->depth();
  }

  ~ShapeI() { Shape::deallocate(s); }

  ShapeI(const ShapeI& sh)
      : sol(sh.sol),
        shape_size(sh.shape_size),
        shape_height(sh.shape_height),
        node(sh.node),
        s(Shape::copy(sh.s)) {}

  ShapeI& operator=(const ShapeI& sh) {
    if (this != &sh) {
      Shape::deallocate(s);
      s = Shape::copy(sh.s);
      sol = sh.sol;
      shape_size = sh.shape_size;
      shape_height = sh.shape_height;
      node = sh.node;
    }
    return *this;
  }
};

/// less operator needed for the map
struct CompareShapes {
 public:
  bool operator()(const ShapeI& n1, const ShapeI& n2) const {
    // if (n1.sol > n2.sol) return false;
    // if (n1.sol < n2.sol) return true;

    const Shape& s1 = *n1.s;
    const Shape& s2 = *n2.s;

    if (n1.shape_height < n2.shape_height) return true;
    if (n1.shape_height > n2.shape_height) return false;

    for (int i = 0; i < n1.shape_height; ++i) {
      if (s1[i].l < s2[i].l) return false;
      if (s1[i].l > s2[i].l) return true;
      if (s1[i].r < s2[i].r) return true;
      if (s1[i].r > s2[i].r) return false;
    }
    return false;
  }
};


/// convert to a vector of shape information
static std::vector<ShapeInfo> toShapeVector(std::multiset<ShapeI, CompareShapes>&& mset) {

  std::vector<ShapeInfo> shapes;
  shapes.reserve(mset.size() / 5); /// arbitrary

  auto it = mset.begin(); auto end = mset.end();

  while (it != end) {

    auto upper = mset.upper_bound(*it);

    shapes.push_back({it->sol, it->shape_size, it->shape_height, {it->node}, it->s});
    for (++it; it != upper; ++it) {
      shapes[shapes.size() - 1].nodes.push_back(it->node);
    }
  }

  return shapes;
}

/// Loop through all nodes and add them to the multimap
static std::multiset<ShapeI, CompareShapes>
collectShapes(NodeTree& nt) {

  auto& na = nt.getNA();
  QHash<VisualNode*, int> nSols;

  auto getNoOfSolutions = [&na, &nSols] (VisualNode* n) -> int {
    int nSol = 0;
    switch (n->getStatus()) {
        case SOLVED:
          nSol = 1;
        break;
        case FAILED:
        case SKIPPED:
        case UNDETERMINED:
          nSol = 0;
        break;
        case BRANCH:
          nSol = 0;
          for (int i=n->getNumberOfChildren(); i--;) {
            nSol += nSols.value(n->getChild(na,i));
          }
        break;
        default: break;    /// To avoid compiler warnings
    }

    return nSol;
  };

  std::multiset<ShapeI, CompareShapes> res;

  auto action = [&res, &nSols, getNoOfSolutions](VisualNode* n) {

    auto nSol = getNoOfSolutions(n);

    nSols[n] = nSol;
    res.insert(detail::ShapeI(nSol,n));
  };

  auto root = nt.getRoot();

  root->unhideAll(na);
  root->layout(na);

  utils::applyToEachNodePO(nt, action);

  return res;
}


}

struct Subtree {
  VisualNode* root;
  NodeTree* nt;
};

std::vector<ShapeInfo> runSimilarShapes(NodeTree& nt) {
    return detail::toShapeVector(detail::collectShapes(nt));
}


struct CompareSubtrees {

private:



public:
 bool operator()(std::shared_ptr<const Subtree> n1, std::shared_ptr<const Subtree> n2) const {

  using std::make_shared;
  using std::shared_ptr;

  std::stack<shared_ptr<const Subtree>> stack1;
  std::stack<shared_ptr<const Subtree>> stack2;

  stack1.push(n1); stack2.push(n2);

  while (stack1.size() > 0) {

    auto s1 = stack1.top(); stack1.pop();
    auto s2 = stack2.top(); stack2.pop();

    auto nkids1 = s1->root->getNumberOfChildren();
    auto nkids2 = s2->root->getNumberOfChildren();

    if (nkids1 > nkids2) return true;
    if (nkids1 < nkids2) return false;

    for (auto i = 0u; i < nkids1; ++i) {
      auto child1 = s1->root->getChild(s1->nt->getNA(), i);
      auto child2 = s2->root->getChild(s2->nt->getNA(), i);

      Subtree temp1{child1, s1->nt};
      Subtree temp2{child2, s2->nt};
      stack1.push(make_shared<Subtree>(temp1));
      stack2.push(make_shared<Subtree>(temp2));
    }

  }

  return false;
 }
};

using std::vector;

vector<vector<VisualNode*>> runIdenticalSubtrees(NodeTree& nt) {

  std::multiset<std::shared_ptr<Subtree>, CompareSubtrees> mset;

  auto action = [&mset, &nt](VisualNode* n) {
    Subtree s{n, &nt};
    mset.insert(std::make_shared<Subtree>(s));
  };

  utils::applyToEachNodePO(nt, action);

  vector<vector<VisualNode*>> result;

  auto it = mset.begin(); auto end = mset.end();

  while (it != end) {

    auto upper = mset.upper_bound(*it);

    std::vector<VisualNode*> group;

    while (it != upper) {
      group.push_back((*it)->root);
      ++it;
    }

    result.push_back(group);
  }

  return result;


}




}}
