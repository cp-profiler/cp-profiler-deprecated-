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

#include "nogood_dialog.hh"
#include "treecanvas.hh"
#include "data.hh"
#include "execution.hh"
#include <QDebug>
#include <QHBoxLayout>
#include <QStandardItemModel>

const int NogoodDialog::DEFAULT_WIDTH = 600;
const int NogoodDialog::DEFAULT_HEIGHT = 400;

// Get the most specific path component that is still in the user model
QString getPathHead(QString& path) {
  QStringList pathSplit = path.split(";");

  QString mzn_file;
  QString previousHead;
  int i=0;
  do {
    QString path_head = pathSplit[i];
    QStringList head = path_head.split(":");
    QString head_file;
    if(head.size() > 0) {
      if(i==0) mzn_file = head[0];
      head_file = head[0];
    }

    if(head_file != mzn_file)
        return previousHead;

    previousHead = path_head;
    i++;
  } while(i < pathSplit.size());

  return previousHead;
}

NogoodDialog::NogoodDialog(
    QWidget* parent, TreeCanvas& tc, const std::vector<int>& selected_nodes,
    const std::unordered_map<int64_t, std::string>& sid2nogood, int64_t root_gid)
    : QDialog(parent), _tc(tc), _sid2nogood(sid2nogood), _root_gid(root_gid) {
  _model = new QStandardItemModel(0, 2, this);

  _nogoodTable = new QTableView(this);
  _nogoodTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _nogoodTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  QStringList tableHeaders;
  tableHeaders << "node id"
               << "clause";
  _model->setHorizontalHeaderLabels(tableHeaders);

  _nogoodTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  _nogoodTable->setSortingEnabled(true);
  _nogoodTable->horizontalHeader()->setStretchLastSection(true);

  connect(_nogoodTable, SIGNAL(doubleClicked(const QModelIndex&)), this,
          SLOT(selectNode(const QModelIndex&)));

  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

  auto layout = new QVBoxLayout(this);

  populateTable(selected_nodes);

  _proxy_model = new MyProxyModel(this);
  _proxy_model->setSourceModel(_model);

  _nogoodTable->setModel(_proxy_model);

  layout->addWidget(_nogoodTable);

  auto heatmapButton = new QPushButton("Heatmap");
  connect(heatmapButton, &QPushButton::clicked, this, [=](){
      QModelIndexList selection = _nogoodTable->selectionModel()->selectedRows();

      auto sid2info = _tc.getExecution().getInfo();
      const NameMap& nm = _tc.getExecution().getNameMap();
      std::unordered_map<std::string, int> con_ids;

      int max_count = 0;
      for(int i=0; i<selection.count(); i++) {
          size_t row = size_t(selection.at(i).row());
          int64_t sid = sids[row];
          auto info_item = sid2info.find(sid);
          if(info_item != sid2info.end()) {
              std::string info_text = *info_item->second;
              size_t start = info_text.find("[");
              size_t end = info_text.find("]");
              QString cons = QString::fromStdString(info_text.substr(start+1, end-start-1));
              QStringList consSplit = cons.split(",");

              for(QString& s : consSplit) {
                  std::string con_string = s.toStdString();
                  int count = 0;
                  if(con_ids.find(con_string) == con_ids.end()) {
                      con_ids[con_string] = 1;
                      count = 1;
                  } else {
                      con_ids[con_string]++;
                      count = con_ids[con_string];
                  }

                  max_count = count > max_count ? count : max_count;
              }

          } else {
              qDebug() << "Can't match sid to info_item.\n";
          }
      }

      int bucket = int(ceil(255.0/double(max_count+1)));

      QStringList highlight_url;
      highlight_url << "<a href=\"highlight://?";
      for(auto it : con_ids) {
        QString path = QString::fromStdString(nm.getPath(it.first));
        QString path_head = getPathHead(path);
        QStringList location_etc = path_head.split(":");
        int count = it.second;

        if(location_etc.count() >= 5) {
          QStringList newLoc;
          for(int i=0; i<5; i++) newLoc << location_etc[i];
          int val = (1 + count) * bucket;
          newLoc << QString::number(val <= 255 ? val : 255);
          highlight_url << newLoc.join(":") << ";";
        }
      }
      highlight_url << "\">Heatmap ("<< QString::number(_root_gid) << ")</a>";

      std::string thestring = highlight_url.join("").toStdString();
      _tc.emitShowNogoodToIDE(thestring);

  });

  layout->addWidget(heatmapButton);
}

NogoodDialog::~NogoodDialog() {}


void NogoodDialog::populateTable(const std::vector<int>& selected_nodes) {
  int row = 0;
  sids.clear();
  auto sid2info = _tc.getExecution().getInfo();
  for (auto it = selected_nodes.begin(); it != selected_nodes.end(); it++) {
    int gid = *it;

    int64_t sid = _tc.getExecution().getData().gid2sid(gid);

    /// TODO(maxim): check if a node is a failure node

    auto ng_item = _sid2nogood.find(sid);
    if (ng_item == _sid2nogood.end()) {
      continue;  /// nogood not found
    }

    std::string clause = _tc.getExecution().getNameMap().replaceNames(ng_item->second);

    _model->setItem(row, 0, new QStandardItem(QString::number(gid)));
    _model->setItem(row, 1, new QStandardItem(clause.c_str()));
    sids.push_back(sid);

    row++;
  }

  _nogoodTable->resizeColumnsToContents();
}

void NogoodDialog::selectNode(const QModelIndex& index) {
  int gid = index.sibling(index.row(), 0).data().toInt();
  qDebug() << "gid: " << gid;
  _tc.navigateToNodeById(gid);
}
