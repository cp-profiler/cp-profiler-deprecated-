/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NODE_INFO_DIALOG_HH
#define NODE_INFO_DIALOG_HH

#include <QDialog>
#include <QTextEdit>


class NodeInfoDialog : public QDialog {
  Q_OBJECT

private:
  static const int DEFAULT_WIDTH;
  static const int DEFAULT_HEIGHT;

  QTextEdit _textField;

public:

  NodeInfoDialog(QWidget* parent, const std::string& text);
  ~NodeInfoDialog();

};

#endif