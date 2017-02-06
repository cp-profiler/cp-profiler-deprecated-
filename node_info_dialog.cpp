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

const int NodeInfoDialog::DEFAULT_WIDTH = 600;
const int NodeInfoDialog::DEFAULT_HEIGHT = 400;

NodeInfoDialog::NodeInfoDialog(QWidget* parent, const std::string& text)
: QDialog(parent) {

  auto textField = new QTextEdit(this);

  textField->setText(text.c_str());
  textField->setReadOnly(true);

  auto layout = new QVBoxLayout(this);
  layout->addWidget(textField);

#ifdef MAXIM_DEBUG

  auto label_lo = new QHBoxLayout(this);

  auto edit_text = new QLabel("Change label: ");
  label_lo->addWidget(edit_text);


  auto label_edit = new QLineEdit(this);
  label_lo->addWidget(label_edit);
  connect(label_edit, &QLineEdit::returnPressed, [label_edit, this] () {
    emit changeLabel(label_edit->text());
    close();
  });

  layout->addLayout(label_lo);
#endif

  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

}