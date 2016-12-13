#pragma once

#include <QDialog>
#include <QLabel>
#include <memory>

#include "subtree_analysis.hh"

class VisualNode;
class NodeTree;
class QAbstractScrollArea;
class QGraphicsScene;
class QGraphicsView;
class QHBoxLayout;

namespace cpprofiler {
namespace analysis {

class SubtreeCanvas;
struct ShapeInfo;

class HistogramWindow : public QDialog {

    std::unique_ptr<SubtreeCanvas> m_SubtreeCanvas;

    void initSharedInterface(QAbstractScrollArea* sa);

protected:

    bool labelSensitive   = false;
    bool shapes_cached    = false;
    bool subtrees_cached  = false;

    SimilarityType  simType = SimilarityType::SUBTREE;

    NodeTree& node_tree;
    std::unique_ptr<QGraphicsScene> m_scene;
    QGraphicsView* hist_view;

    /// the result of similar shapes analysis
    std::vector<ShapeInfo> shapes;
    /// the result of identical subree analysis
    GroupsOfNodes_t m_identicalGroups;


    QHBoxLayout* settingsLayout;
    QHBoxLayout* filtersLayout;

#ifdef MAXIM_DEBUG
    QLabel debug_label{"debug info"};
#endif


public:
    HistogramWindow(NodeTree& nt);

    virtual ~HistogramWindow();

    virtual void highlightSubtrees(VisualNode*);



};




}
}