#ifndef CPPROFILER_SUBTREE_CANVAS_HH
#define CPPROFILER_SUBTREE_CANVAS_HH

#include <QWidget>
#include <memory>

class QAbstractScrollArea;
class NodeTree;
class VisualNode;
class QPaintEvent;

namespace cpprofiler {
namespace analysis {

/// "Connects" to a tree and shows a part of it
class SubtreeCanvas : public QWidget {
Q_OBJECT
  std::unique_ptr<QAbstractScrollArea> m_ScrollArea;
  const NodeTree& m_NodeTree;
  VisualNode* cur_node = nullptr;

  void paintEvent(QPaintEvent* event) override;
public:
  SubtreeCanvas(std::unique_ptr<QAbstractScrollArea>&& sa, const NodeTree& nt);
  void showSubtree(VisualNode* node);

  ~SubtreeCanvas();
};


}}

#endif