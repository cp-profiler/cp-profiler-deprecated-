
#include "cpprofiler/analysis/similar_shapes.hh"

using cpprofiler::analysis::SimilarShapesWindow;

namespace subtree_comparison {


static void compareExecutions(Execution& ex, const Execution& ex1,
                            const Execution& ex2) {

  /// ** construct a tree out of nodes from ex1 and ex2 **

  /// target
  auto& nt = ex.nodeTree();
  auto& na = nt.getNA();
  auto* root = nt.getRoot();

  /// create two children and copy the trees into them
  tree_utils::addChildren(root, nt, 2);

  auto* l_child = root->getChild(na, 0);
  auto* r_child = root->getChild(na, 1);

  /// left source
  const auto* l_root_s = ex1.nodeTree().getRoot();
  /// right source
  const auto* r_root_s = ex2.nodeTree().getRoot();

  tree_utils::copyTree(l_child, nt, l_root_s, ex1.nodeTree());
  tree_utils::copyTree(r_child, nt, r_root_s, ex2.nodeTree());

  /// maps nodes to execution id
  std::unordered_map<VisualNode*, int> node2ex_id; 

  tree_utils::applyToEachNode(nt, l_child, [&node2ex_id](VisualNode* n){
    node2ex_id[n] = 0;
  });

  tree_utils::applyToEachNode(nt, r_child, [&node2ex_id](VisualNode* n){
    node2ex_id[n] = 1;
  });

  /// run similar subtree analysis
  auto shapes_window = new SimilarShapesWindow{nt};
  shapes_window->show();


}





}