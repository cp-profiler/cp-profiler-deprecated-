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