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
#ifndef NOGOOD_DIALOG_HH
#define NOGOOD_DIALOG_HH

#include <QDialog>
#include <QDebug>
#include <QTableWidget>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <cstdint>
#include <QSortFilterProxyModel>

#include "nogoodtable.hh"

class TreeCanvas;
class QStandardItemModel;

class NogoodDialog : public QDialog {
  Q_OBJECT

 private:
  static const int DEFAULT_WIDTH;
  static const int DEFAULT_HEIGHT;

  TreeCanvas& _tc;

  // QTableWidget* _nogoodTable;
  NogoodTableView* _nogoodTable;
  const std::unordered_map<int, std::string>& _sid2nogood;

  QStandardItemModel* _model;
  NogoodProxyModel* _proxy_model;

 private:
  void populateTable(const std::vector<int>& selected_gids);

  void updateNogood(int row, QString newNogood);

 private Q_SLOTS:

  void selectNode(const QModelIndex& index);

 public:
  /// Create a nogood dialog with nogoods for selected nodes
  NogoodDialog(QWidget* parent, TreeCanvas& tc,
               const std::vector<int>& selected,
               const std::unordered_map<int, std::string>& sid2nogood);

  ~NogoodDialog();
};

#endif
