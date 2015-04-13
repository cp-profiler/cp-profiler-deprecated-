#include "solver_tree_dialog.hh"
#include "nodewidget.hh"

SolverTreeDialog::SolverTreeDialog(ReceiverThread* receiver, const CanvasType type, Gist* gist)
: BaseTreeDialog(receiver, type, gist)
{


  hbl->addWidget(new QLabel("Depth:"));
  depthLabel = new QLabel("0");
  hbl->addWidget(depthLabel);

  hbl->addWidget(new NodeWidget(SOLVED));
  solvedLabel = new QLabel("0");
  hbl->addWidget(solvedLabel);

  hbl->addWidget(new NodeWidget(FAILED));
  failedLabel = new QLabel("0");
  hbl->addWidget(failedLabel);

  hbl->addWidget(new NodeWidget(BRANCH));
  choicesLabel = new QLabel("0");
  hbl->addWidget(choicesLabel);

  hbl->addWidget(new NodeWidget(UNDETERMINED));
  openLabel = new QLabel("0");
  hbl->addWidget(openLabel);
  
}

SolverTreeDialog::~SolverTreeDialog() {

}

/// SLOTS 

void
SolverTreeDialog::statusChanged(VisualNode*, const Statistics& stats, bool finished) {

  depthLabel->setNum(stats.maxDepth);
  solvedLabel->setNum(stats.solutions);
  failedLabel->setNum(stats.failures);
  choicesLabel->setNum(stats.choices);
  openLabel->setNum(stats.undetermined);

  statusChangedShared(finished);

}