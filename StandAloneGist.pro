#-------------------------------------------------
#
# Project created by QtCreator 2014-03-28T15:13:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport


TARGET = StandAloneGist
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    heap.cpp \
    nodewidget.cpp \
    drawingcursor.cpp \
    treecanvas.cpp \
    visualnode.cpp \
    nodestats.cpp \
    preferences.cpp \
    qtgist.cpp \
    spacenode.cpp \
    node.cpp \
    data.cpp \
    recieverthread.cpp \
    treebuilder.cpp \
    sqlite/sqlite3.c
    



HEADERS  += mainwindow.h \
    qtgist.hh \
    treecanvas.hh \
    visualnode.hh \
    spacenode.hh \
    node.hh \
    node.hpp \
    spacenode.hpp \
    visualnode.hpp \
    heap.hpp \
    nodestats.hh \
    preferences.hh \
    nodewidget.hh \
    drawingcursor.hh \
    drawingcursor.hpp \
    nodecursor.hh \
    nodecursor.hpp \
    layoutcursor.hh \
    layoutcursor.hpp \
    nodevisitor.hh \
    nodevisitor.hpp \
    zoomToFitIcon.hpp \
    data.hh \
    recieverthread.hh \
    treebuilder.hh
    sqlite/sqlite3.h

FORMS    +=

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../../../usr/local/lib/release/ -lzmq
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../../../usr/local/lib/debug/ -lzmq
else:unix: LIBS += -L$$PWD/../../../../../../usr/local/lib/ -lzmq -ldl

INCLUDEPATH += $$PWD/../../../../../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../../../../../usr/local/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../../../usr/local/lib/release/libzmq.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../../../usr/local/lib/debug/libzmq.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../../../usr/local/lib/release/zmq.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../../../usr/local/lib/debug/zmq.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../../../../../../usr/local/lib/libzmq.a
