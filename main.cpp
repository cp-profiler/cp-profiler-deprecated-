/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "gistmainwindow.h"
#include "globalhelper.hh"
#include <QApplication>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    
    QCoreApplication::setApplicationName("cpprof");
    QCoreApplication::setApplicationVersion("0.1");

    GlobalParser clParser;

    clParser.process(a);

    if (GlobalParser::isSet(GlobalParser::version_option()) ||
        GlobalParser::isSet(GlobalParser::help_option())) {
      return 0;
    }

    GistMainWindow w;
    w.show();

    QObject::connect(&a, SIGNAL(focusChanged(QWidget*,QWidget*)),
                      w.getGist(), SLOT(onFocusChanged(QWidget*,QWidget*)));

    return a.exec();
    
}
