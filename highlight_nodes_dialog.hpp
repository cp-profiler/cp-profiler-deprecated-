#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>

#include <treecanvas.hh>

class HighlightNodesDialog : public QDialog {
  Q_OBJECT

public:
  HighlightNodesDialog(QWidget* parent): QDialog{parent} {

  	// auto tc = static_cast<TreeCanvas*>(parent);
  	auto tc = reinterpret_cast<TreeCanvas*>(parent);

    auto layout = new QVBoxLayout();

    auto reset_btn = new QPushButton{"reset", this};
    auto has_info_btn = new QPushButton{"info item", this};
    auto nogoods_info_btn = new QPushButton{"nodes failed due to nogoods", this};

    connect(reset_btn, &QPushButton::clicked, tc, &TreeCanvas::resetNodesHighlighting);
    connect(has_info_btn, &QPushButton::clicked, tc, &TreeCanvas::highlightNodesWithInfo);
    connect(nogoods_info_btn, &QPushButton::clicked, tc, &TreeCanvas::highlightFailedByNogoods);

    layout->addWidget(reset_btn);
    layout->addWidget(has_info_btn);
    layout->addWidget(nogoods_info_btn);

    this->setLayout(layout);

  }

};