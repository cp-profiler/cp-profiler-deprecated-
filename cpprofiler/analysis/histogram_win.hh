#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <memory>
#include <unordered_map>

#include "subtree_analysis.hh"

class VisualNode;
class NodeTree;
class QAbstractScrollArea;
class QGraphicsScene;
class QGraphicsView;
class QHBoxLayout;
class Execution;

namespace cpprofiler {
namespace analysis {

class SubtreeCanvas;
struct SubtreeInfo;
struct ShapeInfo;
  
enum class LabelOption {
    IGNORE_LABEL, VARS, FULL
};

struct HistogramSettings {

    LabelOption label_opt = LabelOption::IGNORE_LABEL;
    bool hideNotHighlighted = true;
    bool keepSubsumed = false;
};

template<typename T>
using VecPair = std::pair<std::vector<T>, std::vector<T>>;

template<typename T>
using VecVec = std::vector<std::vector<T>>;

VecPair<std::string> getLabelDiff(const Execution& ex, const VisualNode* n1,
                                  const VisualNode* n2);

std::vector<std::string> getLabelDiff(const Execution& ex,
                                      const std::vector<VisualNode*>& vec);

class HistogramWindow : public QDialog {

    std::unique_ptr<SubtreeCanvas> m_SubtreeCanvas;

    void initSharedInterface(QAbstractScrollArea* sa);

protected:

    HistogramSettings settings;
    bool shapes_cached    = false;
    bool subtrees_cached  = false;

    SimilarityType  simType = SimilarityType::SHAPE;

    Execution& execution;
    NodeTree& node_tree;
    std::unique_ptr<QGraphicsScene> m_scene;
    QGraphicsView* hist_view;

    QLineEdit labelDiff;

    std::unordered_map<const ShapeRect*, std::unique_ptr<SubtreeInfo>> rect_to_si;

    /// the result of similar shapes analysis
    // std::vector<ShapeInfo> shapes;
    GroupsOfNodes_t shapes;
    /// the result of identical subree analysis
    GroupsOfNodes_t m_identicalGroups;

    GroupsOfNodes_t groups_shown;

    QHBoxLayout* settingsLayout;
    QHBoxLayout* filtersLayout;
    QHBoxLayout* miscLayout;

#ifdef MAXIM_DEBUG
    QLabel debug_label{"debug info"};
#endif

    void workoutLabelDiff(const SubtreeInfo* const si);
    void highlightSubtrees(VisualNode*);

public:
    HistogramWindow(Execution& nt);
    virtual ~HistogramWindow();

    void handleRectClick(const ShapeRect* rect);
};




}
}
