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

#include "cmp_tree_dialog.hh"
#include "nodewidget.hh"
#include "treecomparison.hh"
#include "treecanvas.hh"
#include "data.hh"

using std::string;

CmpTreeDialog::CmpTreeDialog(QWidget* parent, Execution* execution, bool with_labels,
                             const Execution& ex1, const Execution& ex2)
    : QDialog{parent} {

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

  m_Cmp_result = treecomparison::compare(m_Canvas.get(), ex1, ex2, with_labels);

  /// sort the pentagons by nodes diff:
  m_Cmp_result->sortPentagons();

  mergedLabel->setNum(m_Cmp_result->get_no_pentagons());

  setAttribute( Qt::WA_DeleteOnClose );

  m_Canvas->setCurrentNode(m_Canvas->getExecution()->nodeTree().getRoot());

  resize(500, 400);
  show();
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
CmpTreeDialog::statusChanged(VisualNode*, const Statistics&, bool finished) {

  if (finished) {
    /// add total time to 'Done' label
    QString t;
    unsigned long long totalTime = m_Canvas->getTotalTime();

    const int MILLION = 1000000;
    float seconds = (float)totalTime / MILLION; /// microseconds to seconds

    t.setNum(seconds);
    statusBar->showMessage("Done in " + t + "s");

    qDebug() << "Done in " + t + "s";

    /// no need to change stats after done
    disconnect(m_Canvas.get(), SIGNAL(statusChanged(VisualNode*, const Statistics&, bool)),
          this, SLOT(statusChanged(VisualNode*, const Statistics&, bool)));

  } else {
    statusBar->showMessage("Searching");
  }

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


static string empty_string = "";

// string& getNogoodById(int ng_id, const Execution& e) {
string& getNogoodById(int64_t ng_id, const std::unordered_map<int64_t, string>& ng_map) {


  string& nogood = empty_string;

  auto maybe_nogood = ng_map.find(ng_id);

  if (maybe_nogood != ng_map.end()){
    nogood = maybe_nogood->second;
  }

  return nogood;
}

void
CmpTreeDialog::saveComparisonStatsTo(const QString& file_name) {
  if (file_name != "") {
        QFile outputFile(file_name);
        if (outputFile.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&outputFile);

            auto ng_stats = m_Cmp_result->responsible_nogood_stats();
            auto nogood_map = m_Cmp_result->left_execution().getNogoods();

            out << "id,occur,score,nogood\n";

            for (auto& ng : ng_stats) {

              out << ng.first << ",";
              out << ng.second.occurrence << ",";
              out << ng.second.search_eliminated << ",";

              string& nogood = getNogoodById(ng.first, nogood_map);

              out << nogood.c_str() << "\n";
            }

            qDebug() << "writing comp stats to the file: " << file_name;
        } else {
          qDebug() << "could not open the file: " << file_name;
        }
    }
}

void
CmpTreeDialog::saveComparisonStats() {
  saveComparisonStatsTo("temp_stats.txt");
}

/// *************** Pentagon List Window ****************


void initNogoodTable(QTableWidget& ng_table) {
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
  auto nogood_map = cmp_result.left_execution().getNogoods();

  _nogoodTable.setRowCount(nogoods.size());

  for (auto i = 0u; i < nogoods.size(); i++) {

    int ng_id = nogoods[i]; /// is this sid of gid???
    _nogoodTable.setItem(i, 0, new QTableWidgetItem(QString::number(ng_id)));

    string& nogood = getNogoodById(ng_id, nogood_map);

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



QString infoToNogoodStr(const string& info) {
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

  auto ng_dialog = new QDialog(this);
  auto ng_layout = new QVBoxLayout();

  ng_dialog->resize(600, 400);
  ng_dialog->setLayout(ng_layout);
  ng_dialog->show();


  auto ng_table = new QTableWidget(ng_dialog);
  ng_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ng_table->setColumnCount(4);
  ng_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ng_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  ng_table->setHorizontalHeaderLabels({"Id", "Occurrence",
                                       "Reduction Total", "Literals"});

  ng_layout->addWidget(ng_table);


  auto save_btn = new QPushButton("Save as", ng_dialog);

  connect(save_btn, &QPushButton::clicked, this, [this](){
    saveComparisonStatsTo("temp_stats.txt");
  });

  ng_layout->addWidget(save_btn);

  /// *** edit table ***

  auto ng_stats = m_Cmp_result->responsible_nogood_stats();

  /// map to vector
  std::vector<std::pair<int, NogoodCmpStats> > ng_stats_vector;
  ng_stats_vector.reserve(ng_stats.size());

  for (auto ng : ng_stats) {
    ng_stats_vector.push_back(ng);
  }

  std::sort(ng_stats_vector.begin(), ng_stats_vector.end(),
    [](const std::pair<int, NogoodCmpStats>& lhs, const std::pair<int, NogoodCmpStats>& rhs){
      // return lhs.second.occurrence > rhs.second.occurrence;
      return lhs.second.search_eliminated > rhs.second.search_eliminated;
  });

  ng_table->setRowCount(ng_stats.size());

  auto& nogood_map = m_Cmp_result->left_execution().getNogoods();

  qDebug() << "map2: " << (void*)(&nogood_map);

  int row = 0;
  for (auto ng : ng_stats_vector) {

    ng_table->setItem(row, 0, new QTableWidgetItem(QString::number(ng.first)));
    ng_table->setItem(row, 1, new QTableWidgetItem(QString::number(ng.second.occurrence)));

    string& nogood = getNogoodById(ng.first, nogood_map);

    ng_table->setItem(row, 2, new QTableWidgetItem(QString::number(ng.second.search_eliminated)));
    ng_table->setItem(row, 3, new QTableWidgetItem(nogood.c_str()));

    // if (row > 100) break;

    ++row;
  }

  ng_table->resizeColumnsToContents();

  auto total_reduced = m_Cmp_result->get_total_reduced();
  auto total_nodes = m_Cmp_result->right_execution().getData()->size();

  auto reduction_label = QString{"Nodes reduced: "} +
                         QString::number(total_reduced) +
                         QString{" out of "} +
                         QString::number(total_nodes);

  ng_layout->addWidget(new QLabel{reduction_label});

}

/// ******************************************************
