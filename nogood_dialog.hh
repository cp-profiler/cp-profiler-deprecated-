#ifndef NOGOOD_DIALOG_HH
#define NOGOOD_DIALOG_HH

#include <QDialog>
#include <QTableWidget>
#include <unordered_map>
#include <vector>
#include <QStandardItemModel>

class TreeCanvas;


class NogoodDialog : public QDialog {
  Q_OBJECT

private:

  static const int DEFAULT_WIDTH;
  static const int DEFAULT_HEIGHT;

  TreeCanvas& _tc;

  // QTableWidget* _nogoodTable;
  QTableView* _nogoodTable;
  const std::unordered_map<unsigned long long, std::string>& _sid2nogood;

  QStandardItemModel* _model;

private:

  void populateTable();

private Q_SLOTS:

  void selectNode(const QModelIndex & index);

public:

  NogoodDialog(QWidget* parent, TreeCanvas& tc,
    const std::unordered_map<unsigned long long, std::string>& sid2nogood);

  ~NogoodDialog();


};



#endif