#pragma once

#include <vector>

class NodeTree;
class VisualNode;
class Shape;

namespace cpprofiler {
namespace analysis {

struct ShapeInfo {
  int sol;
  int size;
  int height;
  std::vector<VisualNode*> nodes;
  Shape* s;
};

std::vector<ShapeInfo> runSimilarShapes(NodeTree& nt);

std::vector<std::vector<VisualNode*>> runIdenticalSubtrees(NodeTree& nt);

}}