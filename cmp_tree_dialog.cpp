/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <algorithm>
#include <cmath>
#include "third-party/json.hpp"

#include "globalhelper.hh"
#include "cmp_tree_dialog.hh"
#include "nodewidget.hh"
#include "treecomparison.hh"
#include "treecanvas.hh"
#include "execution.hh"
#include "nodetree.hh"
#include "data.hh"
#include "nogoodtable.hh"
#include "libs/perf_helper.hh"
#include "cpprofiler/utils/nogood_subsumption.hh"

using std::string;

CmpTreeDialog::CmpTreeDialog(QWidget* parent, Execution* execution, bool with_labels,
                             const Execution& ex1, const Execution& ex2)
    : QDialog{parent}, expand_expressions(true) {

  qDebug() << "new CmpTreeDialog";

  auto main_layout = new QHBoxLayout();
  layout = new QGridLayout(this);
  auto nc_layout = new QVBoxLayout();
  auto status_layout = new QVBoxLayout();

  main_layout->addLayout(status_layout);

  auto scrollArea = new QAbstractScrollArea(this);

  m_Canvas.reset(new TreeCanvas(execution, layout, scrollArea->viewport()));

  layout->addWidget(scrollArea, 0, 0, 1, 1);
  layout->addWidget(m_Canvas->scaleBar(), 0, 1, Qt::AlignHCenter);

  scrollArea->viewport()->setLayout(nc_layout);

  connect(scrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_Canvas.get(), SLOT(scroll(void)));
  connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
            m_Canvas.get(), SLOT(scroll(void)));

  nc_layout->addWidget(m_Canvas.get());

  auto menuBar = new QMenuBar(this);
  auto nodeMenu = menuBar->addMenu(tr("&Node"));
  auto analysisMenu = menuBar->addMenu(tr("&Analysis"));

  addActions(nodeMenu, analysisMenu);

    // Don't add the menu bar on Mac OS X
  #ifndef Q_WS_MAC
    layout->setMenuBar(menuBar);
  #endif

  statusBar = new QStatusBar(this);

  QWidget* stw = new QWidget();
  statusBar->addPermanentWidget(stw);
  statusBar->showMessage("Ready");
  layout->addWidget(statusBar);

  auto hbl = new QHBoxLayout();
  hbl->setContentsMargins(0,0,0,0);
  hbl->addWidget(new NodeWidget(MERGING));

  auto mergedLabel = new QLabel("0");
  hbl->addWidget(mergedLabel);

  stw->setLayout(hbl);

  perfHelper.begin("comparison");

  m_Cmp_result = treecomparison::compareTrees(*m_Canvas, ex1, ex2, with_labels);

  perfHelper.end();

  /// sort the pentagons by nodes diff:
  m_Cmp_result->sortPentagons();

  mergedLabel->setNum(m_Cmp_result->get_no_pentagons());

  setAttribute( Qt::WA_DeleteOnClose );

  m_Canvas->setCurrentNode(m_Canvas->getExecution().nodeTree().getRoot());

  resize(500, 400);
  show();
  m_Canvas->reset();
}

CmpTreeDialog::~CmpTreeDialog() = default;

void
CmpTreeDialog::addActions(QMenu* nodeMenu, QMenu* analysisMenu) {

  /// Navigation

  auto navUp = new QAction("Up", this);
  addAction(navUp);
  nodeMenu->addAction(navUp);
  navUp->setShortcut(QKeySequence("Up"));
  connect(navUp, SIGNAL(triggered()), m_Canvas.get(), SLOT(navUp()));

  auto navDown = new QAction("Down", this);
  addAction(navDown);
  nodeMenu->addAction(navDown);
  navDown->setShortcut(QKeySequence("Down"));
  connect(navDown, SIGNAL(triggered()), m_Canvas.get(), SLOT(navDown()));

  auto navLeft = new QAction("Left", this);
  addAction(navLeft);
  nodeMenu->addAction(navLeft);
  navLeft->setShortcut(QKeySequence("Left"));
  connect(navLeft, SIGNAL(triggered()), m_Canvas.get(), SLOT(navLeft()));

  auto navRight = new QAction("Right", this);
  addAction(navRight);
  nodeMenu->addAction(navRight);
  navRight->setShortcut(QKeySequence("Right"));
  connect(navRight, SIGNAL(triggered()), m_Canvas.get(), SLOT(navRight()));

  auto navRoot = new QAction("Root", this);
  addAction(navRoot);
  nodeMenu->addAction(navRoot);
  navRoot->setShortcut(QKeySequence("R"));
  connect(navRoot, SIGNAL(triggered()), m_Canvas.get(), SLOT(navRoot()));

  /// Note(maxim): Qt::WindowShortcut is default context
  auto navFirstPentagon = new QAction("To first pentagon", this);
  navFirstPentagon->setShortcut(QKeySequence("Ctrl+Shift+1"));
  addAction(navFirstPentagon);
  nodeMenu->addAction(navFirstPentagon);
  connect(navFirstPentagon, SIGNAL(triggered()), this, SLOT(navFirstPentagon()));

  auto navNextPentagon = new QAction("To next pentagon", this);
  navNextPentagon->setShortcut(QKeySequence("Ctrl+Shift+Right"));
  addAction(navNextPentagon);
  nodeMenu->addAction(navNextPentagon);
  connect(navNextPentagon, SIGNAL(triggered()), this, SLOT(navNextPentagon()));

  auto navPrevPentagon = new QAction("To prev pentagon", this);
  navPrevPentagon->setShortcut(QKeySequence("Ctrl+Shift+Left"));
  addAction(navPrevPentagon);
  nodeMenu->addAction(navPrevPentagon);
  connect(navPrevPentagon, SIGNAL(triggered()), this, SLOT(navPrevPentagon()));

  auto labelBranches = new QAction("Label/clear branches", this);
  labelBranches->setShortcut(QKeySequence("L"));
  addAction(labelBranches);
  nodeMenu->addAction(labelBranches);
  connect(labelBranches, SIGNAL(triggered()), m_Canvas.get(), SLOT(labelBranches()));

  auto showInfo = new QAction("Show info", this);
  showInfo->setShortcut(QKeySequence("I"));
  addAction(showInfo);
  nodeMenu->addAction(showInfo);
  connect(showInfo, SIGNAL(triggered()), m_Canvas.get(), SLOT(showNodeInfo()));

  auto showPentagonHist = new QAction("Pentagon list", this);
  analysisMenu->addAction(showPentagonHist);
  connect(showPentagonHist, SIGNAL(triggered()), this, SLOT(showPentagonHist()));

  auto listNogoodsAction = new QAction("List Responsible Nogoods", this);
  analysisMenu->addAction(listNogoodsAction);
  connect(listNogoodsAction, &QAction::triggered, this, &CmpTreeDialog::showResponsibleNogoods);

  auto saveComparisonStats = new QAction("Save comparison stats", this);
  analysisMenu->addAction(saveComparisonStats);
  connect(saveComparisonStats, SIGNAL(triggered()), this, SLOT(saveComparisonStats()));
}

void
CmpTreeDialog::navFirstPentagon() {
  const auto pentagon_items = m_Cmp_result->pentagon_items();

  if (pentagon_items.size() == 0) {
    qDebug() << "warning: pentagons.size() == 0";
    return;
  }

  m_Canvas->setCurrentNode(pentagon_items[0].node);
  m_Canvas->centerCurrentNode();

}

void
CmpTreeDialog::navNextPentagon() {

  m_Canvas->navNextPentagon();

}

void
CmpTreeDialog::navPrevPentagon() {

  m_Canvas->navNextPentagon(true);
}

void
CmpTreeDialog::showPentagonHist() {

  auto pentagon_window = new PentListWindow(this, *m_Cmp_result);
  pentagon_window->createList();
  pentagon_window->show();
}

void
CmpTreeDialog::saveComparisonStatsTo(const QString& file_name) {
  if (file_name == "") return;

  QFile outputFile(file_name);

  if (!outputFile.open(QFile::WriteOnly | QFile::Truncate)) {
    qDebug() << "could not open the file: " << file_name;
    return;
  }

  QTextStream out(&outputFile);

  auto& ng_stats = m_Cmp_result->responsible_nogood_stats();
  const Execution& left_execution = m_Cmp_result->left_execution();

  out << "id, occur, score, nogood\n";

  std::vector<std::pair<int, NogoodCmpStats> > ng_stats_vector;
  ng_stats_vector.reserve(ng_stats.size());

  for (auto ng : ng_stats) {
    ng_stats_vector.push_back(ng);
  }

  std::sort(ng_stats_vector.begin(), ng_stats_vector.end(),
    [](const std::pair<int, NogoodCmpStats>& lhs,
       const std::pair<int, NogoodCmpStats>& rhs) {
      return lhs.second.search_eliminated > rhs.second.search_eliminated;
  });

  utils::SubsumptionFinder sf{left_execution.getNogoods()};

  for (auto& ng : ng_stats_vector) {

    out << ng.first << ", ";
    out << ng.second.occurrence << ", ";
    out << ng.second.search_eliminated << ", ";

    bool to_subsume = false;
    const string nogood = to_subsume ? sf.getSubsumingClauseString(ng.first)
                                     : left_execution.getNogoodBySid(ng.first);

    out << nogood.c_str() << "\n";
  }

  qDebug() << "writing comp stats to the file: " << file_name;

}

void
CmpTreeDialog::saveComparisonStats() {
  saveComparisonStatsTo("temp_stats.txt");
}

/// *************** Pentagon List Window ****************


static void initNogoodTable(QTableWidget& ng_table) {
  ng_table.setEditTriggers(QAbstractItemView::NoEditTriggers);

  ng_table.setColumnCount(3);

  QStringList table_header;
  table_header << "Id" << "Occurrence" << "Literals";
  ng_table.setHorizontalHeaderLabels(table_header);
  ng_table.setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ng_table.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

std::vector<int>
infoToNogoodVector(const string& info) {

  // TODO(maxim): better way for checking if json?
  if (info.size() > 0 && info[0] == '{') {
    auto info_json = nlohmann::json::parse(info);

    auto nogoods = info_json["nogoods"];

    if (nogoods.is_array()) {
      return nogoods;
    }
  }

  return {};
}

void
PentListWindow::populateNogoodTable(const std::vector<int>& nogoods) {

  auto ng_stats = cmp_result.responsible_nogood_stats();
  const Execution& left_execution = cmp_result.left_execution();

  _nogoodTable.setRowCount(nogoods.size());

  for (auto i = 0u; i < nogoods.size(); i++) {

    auto ng_id = nogoods[i]; /// is this sid of gid???
    _nogoodTable.setItem(i, 0, new QTableWidgetItem(QString::number(ng_id)));

    const string nogood = left_execution.getNogoodBySid(ng_id);
    qDebug() << "nogood id " << ng_id << ": " << nogood.c_str();

    int ng_count = ng_stats.at(ng_id).occurrence;

    _nogoodTable.setItem(i, 1, new QTableWidgetItem(QString::number(ng_count)));

    _nogoodTable.setItem(i, 2, new QTableWidgetItem(nogood.c_str()));

  }

  _nogoodTable.resizeColumnsToContents();

}

PentListWindow::PentListWindow(CmpTreeDialog* parent,
                               const ComparisonResult& result)
    : QDialog(parent),
      _pentagonTable{this},
      cmp_result(result),
      _items(result.pentagon_items()) {

  resize(600, 400);

  setAttribute( Qt::WA_DeleteOnClose );

  connect(&_pentagonTable, &QTableWidget::cellDoubleClicked, [this, parent](int row, int) {
    static_cast<CmpTreeDialog*>(parent)->selectPentagon(row);

    auto maybe_info = _items[row].info;

    /// clear nogood view
    _nogoodTable.clearContents();

    if (maybe_info) {
      auto nogoods = infoToNogoodVector(*maybe_info);

      populateNogoodTable(nogoods);
    }
  });

  auto layout = new QVBoxLayout(this);

  _pentagonTable.setEditTriggers(QAbstractItemView::NoEditTriggers);
  _pentagonTable.setSelectionBehavior(QAbstractItemView::SelectRows);

  initNogoodTable(_nogoodTable);

  layout->addWidget(&_pentagonTable);
  layout->addWidget(&_nogoodTable);
}

static QString infoToNogoodStr(const string& info) {
  QString result = "";

  auto info_json = nlohmann::json::parse(info);

  auto nogoods = info_json["nogoods"];

  if (nogoods.is_array() && nogoods.size() > 0) {
    for (auto nogood : nogoods) {
      int ng = nogood;
      result += QString::number(ng) + " ";
    }
  }

  return result;
}

void
PentListWindow::createList()
{


  _pentagonTable.setColumnCount(3);
  _pentagonTable.setRowCount(_items.size());

  QStringList table_header;
  table_header << "Left" << "Right" << "Nogoods involved";
  _pentagonTable.setHorizontalHeaderLabels(table_header);
  _pentagonTable.setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  _pentagonTable.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);


  for (unsigned int i = 0; i < _items.size(); i++) {
    _pentagonTable.setItem(i, 0, new QTableWidgetItem(QString::number(_items[i].l_size)));
    _pentagonTable.setItem(i, 1, new QTableWidgetItem(QString::number(_items[i].r_size)));

    auto maybe_info = _items[i].info;
    if (maybe_info) {

     QString nogood_str = infoToNogoodStr(*maybe_info);

      _pentagonTable.setItem(i, 2, new QTableWidgetItem(nogood_str));
    }
  }
  _pentagonTable.resizeColumnsToContents();

}

void
CmpTreeDialog::selectPentagon(int row) {
  const auto items = m_Cmp_result->pentagon_items();

  auto node = items[row].node;

  //TODO(maxim): this should unhide all nodes above
  // m_Canvas->unhideNode(node); // <- does not work correctly
  m_Canvas->setCurrentNode(node);
  m_Canvas->centerCurrentNode();
  
}

void
CmpTreeDialog::showResponsibleNogoods() {
  enum RNCols {SID_COL = 0, OCCURRENCE_COL, REDUCTION_COL, NOGOOD_COL};

  auto ng_dialog = new QDialog(this);
  auto ng_layout = new QVBoxLayout();
  ng_dialog->resize(600, 400);
  ng_dialog->setLayout(ng_layout);
  ng_dialog->show();

  if (GlobalParser::isSet(GlobalParser::auto_compare)) {
    saveComparisonStatsTo("ng_stats.txt");
    std::cerr << "STATS SAVED\n";
    QCoreApplication::quit();
  }

  auto ng_stats = m_Cmp_result->responsible_nogood_stats();
  const Execution& left_execution = m_Cmp_result->left_execution();
  const NameMap* nm = m_Cmp_result->left_execution().getNameMap();

  auto _model = new QStandardItemModel(0, 2, this);
  _model->setHorizontalHeaderLabels({"Id", "Occurrence", "Reduction Total", "Literals"});
  _model->setColumnCount(4);

  int row = 0;
  for (auto ng : ng_stats) {
    _model->setItem(row, 0, new QStandardItem(QString::number(ng.first)));
    _model->setItem(row, 1, new QStandardItem(QString::number(ng.second.occurrence)));

    const QString nogood = QString::fromStdString(left_execution.getNogoodBySid(ng.first));
    const QString renamed_nogood = nm ? nm->replaceNames(nogood) : nogood;

    _model->setItem(row, 2, new QStandardItem(QString::number(ng.second.search_eliminated)));
    _model->setItem(row, 3, new QStandardItem(renamed_nogood));

    ++row;
  }

  QVector<NogoodProxyModel::Sorter> sorters{
      NogoodProxyModel::SORTER_SID , NogoodProxyModel::SORTER_INT,
      NogoodProxyModel::SORTER_INT , NogoodProxyModel::SORTER_NOGOOD};
  auto ng_table = new NogoodTableView(ng_dialog, _model, sorters,
                                      m_Cmp_result->left_execution(),
                                      SID_COL, NOGOOD_COL);
  ng_table->setSortingEnabled(true);
  ng_table->sortByColumn(REDUCTION_COL, Qt::SortOrder::DescendingOrder);
  ng_layout->addWidget(ng_table);

  auto save_btn = new QPushButton("Save as", ng_dialog);
  ng_layout->addWidget(save_btn);
  connect(save_btn, &QPushButton::clicked, this, [this](){
    saveComparisonStatsTo("temp_stats.txt");
  });

  auto total_reduced = m_Cmp_result->get_total_reduced();
  auto total_nodes = m_Cmp_result->right_execution().getData().size();
  QStringList reduction_label = {"Nodes reduced:", QString::number(total_reduced),
                                 "out of",         QString::number(total_nodes)};
  ng_layout->addWidget(new QLabel{reduction_label.join(" ")});

  if(nm != nullptr) {
    auto buttons = new QHBoxLayout(this);
    auto heatmapButton = new QPushButton("Heatmap");
    heatmapButton->setAutoDefault(false);
    auto showExpressions = new QPushButton("Show/Hide Expressions");
    showExpressions->setAutoDefault(false);
    auto getFlatZinc = new QPushButton("Get FlatZinc");
    getFlatZinc->setAutoDefault(false);

    buttons->addWidget(heatmapButton);
    buttons->addWidget(showExpressions);
    buttons->addWidget(getFlatZinc);
    ng_layout->addLayout(buttons);

    ng_table->connectHeatmapButton(heatmapButton, *m_Canvas.get());
    ng_table->connectShowExpressionsButton(showExpressions);
    ng_table->connectFlatZincButton(getFlatZinc);
  }

  auto filter_layout = new QHBoxLayout();
  filter_layout->addWidget(new QLabel{"Text Include:"});
  auto include_edit = new QLineEdit("");
  filter_layout->addWidget(include_edit);
  filter_layout->addWidget(new QLabel{"Omit:"});
  auto reject_edit = new QLineEdit("");
  filter_layout->addWidget(reject_edit);
  ng_table->connectTextFilter(include_edit, reject_edit);

  if(nm) {
    filter_layout->addWidget(new QLabel{"Location Filter:"});
    auto location_edit = new QLineEdit("");
    ng_table->connectLocationFilter(location_edit);
    filter_layout->addWidget(location_edit);

    auto locationButton = new QPushButton("Get Location");
    locationButton->setAutoDefault(false);
    ng_table->connectLocationButton(locationButton, location_edit);
    filter_layout->addWidget(locationButton);
  }

  ng_layout->addLayout(filter_layout);

  auto subsumlayout = new QHBoxLayout();
  auto subsumbutton = new QPushButton("Simplify nogoods");
  subsumbutton->setAutoDefault(false);
  auto subsumUseAllNogoods = new QCheckBox("Use all nogoods");
  auto subsumUseAllNogoodsApplyFilter = new QCheckBox("Apply filters");
  ng_table->connectSubsumButtons(subsumbutton,
                                 subsumUseAllNogoods,
                                 subsumUseAllNogoodsApplyFilter);
  subsumlayout->addWidget(subsumbutton);
  subsumlayout->addWidget(subsumUseAllNogoods);
  subsumlayout->addWidget(subsumUseAllNogoodsApplyFilter);

  subsumlayout->setStretchFactor(subsumbutton, 2);

  ng_layout->addLayout(subsumlayout);
}

TreeCanvas* CmpTreeDialog::getCanvas() {
  return m_Canvas.get();
}


/// ******************************************************
