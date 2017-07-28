#include "similar_shape_algorithm.hh"
#include <set>

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
          nSol = 0;
        break;
        case SKIPPED:
          nSol = 0;
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

std::vector<ShapeInfo> runSimilarShapes(NodeTree& nt) {
    return detail::toShapeVector(detail::collectShapes(nt));
}




}}