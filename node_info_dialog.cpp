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

const int NodeInfoDialog::DEFAULT_WIDTH = 600;
const int NodeInfoDialog::DEFAULT_HEIGHT = 400;

NodeInfoDialog::NodeInfoDialog(QWidget* parent, const std::string& text)
: QDialog(parent), _textField(this) {

  _textField.setText(text.c_str());
  _textField.setReadOnly(true);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->addWidget(&_textField);

  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

}

NodeInfoDialog::~NodeInfoDialog() {

}