#include "node_info_dialog.hh"
#include <QDebug>
#include <QHBoxLayout>

const int NodeInfoDialog::DEFAULT_WIDTH = 600;
const int NodeInfoDialog::DEFAULT_HEIGHT = 400;

NodeInfoDialog::NodeInfoDialog(QWidget* parent, const std::string& text)
: QDialog(parent), _textField(this) {

  _textField.setText(text.c_str());

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->addWidget(&_textField);

  resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

}

NodeInfoDialog::~NodeInfoDialog() {

}