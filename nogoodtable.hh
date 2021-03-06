#ifndef NOGOODTABLE_H
#define NOGOODTABLE_H

#include "namemap.hh"

#include <qtableview.h>
#include <qsortfilterproxymodel.h>
#include <qboxlayout.h>

class Execution;
class TreeCanvas;
class QPushButton;
class QCheckBox;
class QStandardItemModel;
class NameMap;

class NogoodProxyModel : public QSortFilterProxyModel {
public:
  enum Sorter {SORTER_SID = 0, SORTER_INT, SORTER_NOGOOD};
  explicit NogoodProxyModel(QWidget* parent,
                            const Execution& e,
                            const QVector<Sorter>& sorters);

  void setTextFilterStrings(const QStringList& includeTextFilter,
                            const QStringList& rejectTextFilter);
  void setLocationFilter(const LocationFilter& locationFilter);
  bool filterAcceptsText(const QString& nogood) const;

protected:
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
  bool filterAcceptsRow(int source_row, const QModelIndex&) const;

private:
  QVector<Sorter> _sorters;
  QStringList _include_text_filter;
  QStringList _reject_text_filter;
  LocationFilter _loc_filter;
  const Execution& ex;
  const NameMap* _nm;
  int _sid_col;
  int _nogood_col;
};

class NogoodTableView : public QTableView {
public:
  NogoodTableView(QWidget* parent,
                  QStandardItemModel* model,
                  QVector<NogoodProxyModel::Sorter> sorters,
                  const Execution& e,
                  int sid_col, int nogood_col);

  void addStandardButtons(QWidget* parent, QVBoxLayout* layout,
                          TreeCanvas* canvas, const Execution& e,
                          bool is_comparison = false);

private:
  // Connect buttons and text filters to their respective functionality
  void connectHeatmapButton(const QPushButton* heatmapButton, const TreeCanvas& tc);
  void connectSaveNogoodTable(const QPushButton* saveNogoodsButton,
                              bool is_comparison = false);
  void connectFlatZincButton(const QPushButton* getFlatZinc);
  void connectNogoodRepresentationCheckBoxes(const QCheckBox* changeRep,
                                                   QCheckBox* showSimplified);
  void connectTextFilter(const QLineEdit* include_edit,
                         const QLineEdit* reject_edit);
  void connectLocationFilter(QLineEdit* location_edit);
  void connectLocationButton(const QPushButton* locationButton,
                             QLineEdit* location_edit);
  void connectSubsumButtons(const QPushButton* subsumButton,
                            const QCheckBox* resolution,
                            const QCheckBox* use_all,
                            QCheckBox* apply_filter,
                            const QCheckBox* only_earlier_sids);

  // Find which parts of the table are selected
  QModelIndexList getSelection() const;

  // Read _sid_col of the row (handles mapping)
  NodeUID getUidFromRow(int row) const;
  // Get nogood displayed in the table
  QString getNogoodFromRow(int row) const;
  // Re-select the correct rows (sorting might move rows)
  void updateSelection() const;
  // Set location filter based on selected nodes
  void updateLocationFilter(QLineEdit* location_edit) const;
  // Update _colors
  void updateColors(void);

private slots:
  // Save nogoods table to csv file
  void saveNogoods(bool is_comparison) const;
  // Replace selected nogoods with equivalent FlatZinc
  void showFlatZinc(void);
  // Update the renaming of nogoods, replacing X_INTRODUCED_
  //   with expressions depending on expand_expressions
  void refreshModelRenaming(void);
  // Replace subsumed clauses with their subsuming clause
  void renameSubsumedSelection(const QCheckBox* resolution,
                               const QCheckBox* use_all,
                               const QCheckBox* apply_filter,
                               const QCheckBox* only_earlier_sids);
  // Use self-subsuming resolution
  void renameResolvingSubsumption(void);
  // Get heatmap url for the MiniZincIDE and emit signal
  void getHeatmapAndEmit(const TreeCanvas& tc, bool record) const;

private:
  const Execution& _execution;
  QStandardItemModel* _model;
  NogoodProxyModel* nogood_proxy_model;
  int _sid_col;
  int _nogood_col;
  bool _show_renamed_literals {true};
  bool _show_simplified_nogoods {false};

  std::unordered_map<std::string, QColor> _colors;
};

#endif // NOGOODTABLE_H
