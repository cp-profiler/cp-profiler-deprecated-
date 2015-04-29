#include "gistmainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    QCoreApplication::setApplicationName("cpprof");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser qParser;

    /// Add command line options
    const QCommandLineOption versionOption = qParser.addVersionOption();
    const QCommandLineOption helpOption    = qParser.addHelpOption();

    QCommandLineOption testOption("test", "Terminates right after getting DONE RECEIVING");
    qParser.addOption(testOption);

    qParser.process(a);

    qDebug() << qParser.isSet(testOption);


    if (qParser.isSet(helpOption) || qParser.isSet(versionOption)) {
      return 0;
    }

    GistMainWindow w;
    w.show();

    QObject::connect(&a, SIGNAL(focusChanged(QWidget*,QWidget*)),
                      w.getGist(), SLOT(onFocusChanged(QWidget*,QWidget*)));

    return a.exec();
    
}
