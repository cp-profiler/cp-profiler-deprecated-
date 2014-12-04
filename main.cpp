#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GistMainWindow w;
    w.show();

    QObject::connect(&a, SIGNAL(focusChanged(QWidget*,QWidget*)),
                     w.getGist(), SLOT(onFocusChanged(QWidget*,QWidget*)));

    return a.exec();
}
