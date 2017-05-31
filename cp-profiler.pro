#-------------------------------------------------
#
# Project created by QtCreator 2014-03-28T15:13:15
#
#-------------------------------------------------

# Turn this on for "development mode" (see e.g. webscript.hh)
DEFINES += CP_PROFILER_DEVELOPMENT


QT       += core gui network

# QT += webkitwidgets
# QT += webenginewidgets webchannel

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

QMAKE_CXXFLAGS += -g

CONFIG += c++11

macx: {
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
  LIBS += -lc++
  QMAKE_CXXFLAGS += -stdlib=libc++
}

TARGET = cp-profiler

TEMPLATE = app

SOURCES += main_cpprofiler.cpp

include(cp-profiler.pri)

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../../../usr/local/lib/release/
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../../../usr/local/lib/debug/
else:unix: LIBS += -L$$PWD/../../../../../../usr/local/lib/ -ldl

#CONFIG += link_pkgconfig
#PKGCONFIG += protobuf
