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

#include "node_info_dialog.hh"
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>

const int NodeInfoDialog::DEFAULT_WIDTH = 600;
const int NodeInfoDialog::DEFAULT_HEIGHT = 400;

static QString filterVars(const QString& orig, const QString& filter) {

  if (filter.isEmpty()) return orig;

  QString filtered;

  QStringList var_list = filter.split(',', QString::SkipEmptyParts);

  for (auto& el : var_list) { el = el.trimmed(); }

  QStringList info_lines = orig.split('\n', QString::SkipEmptyParts);

  for (auto& line : info_lines) {
    for (auto& var : var_list) {
      if (line.startsWith(var)) { filtered += line + '\n'; }
    }
  }

  return filtered;

}

NodeInfoDialog::NodeInfoDialog(QWidget* parent, const std::string& text, const QString& filter)
: QDialog(parent), orig_text(text.c_str()) {

  auto textField = new QTextEdit(this);

  textField->setReadOnly(true);

  auto layout = new QVBoxLayout(this);
  layout->addWidget(textField);

  /// var name filter for domains

  auto domain_filter_lo = new QHBoxLayout();
  layout->addLayout(domain_filter_lo);

  auto filter_label = new QLabel{"Filter domains:"};
  domain_filter_lo->addWidget(filter_label);

  auto filter_edit = new QLineEdit(filter);
  domain_filter_lo->addWidget(filter_edit);

  connect(filter_edit, &QLineEdit::returnPressed, [filter_edit, textField, this] () {

    const auto& filter_text = filter_edit->text();

    QString to_show = filterVars(orig_text, filter_text);

    emit this->filterChanged(filter_text);
    textField->setText(to_show);

  });

  filter_edit->setToolTip("variable names separated by commas expected; empty to disable");
  emit filter_edit->returnPressed();


#ifdef MAXIM_DEBUG

  /// text edit field for specifying labels

  auto label_lo = new QHBoxLayout();
  layout->addLayout(label_lo);

  auto edit_text = new QLabel{"Change label:"};
  label_lo->addWidget(edit_text);

  auto label_edit = new QLineEdit(this);
  label_lo->addWidget(label_edit);
  connect(label_edit, &QLineEdit::returnPressed, [label_edit, this] () {
    emit changeLabel(label_edit->text());
    close();
  });

  auto status_cb = new QComboBox();
  status_cb->addItems({"Undetermined", "Branch", "Solved", "Failed"});
  layout->addWidget(status_cb);
  connect(status_cb, &QComboBox::currentTextChanged, [this] (QString str) {
    emit changeStatus(str);
  });

#endif


  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

}