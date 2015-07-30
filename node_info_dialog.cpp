/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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