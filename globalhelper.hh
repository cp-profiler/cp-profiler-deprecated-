/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GLOBALHELPER_HH
#define GLOBALHELPER_HH

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QString>
#include <memory>

class QCoreApplication;

class GlobalParser {

private:

  static GlobalParser* _self;

  static QCommandLineParser clParser;

  static QCommandLineOption helpOption;
  static QCommandLineOption versionOption;

  static QCommandLineOption testOption;
  static QCommandLineOption portOption;

public:


  

public:

  GlobalParser();
  ~GlobalParser();

  static bool isSet(const QCommandLineOption& opt);

  static QString value(const QCommandLineOption& opt);

  static void process(const QCoreApplication & app);

  inline const static QCommandLineOption help_option() { return helpOption; }
  inline const static QCommandLineOption version_option() { return versionOption; }
  inline const static QCommandLineOption test_option() { return testOption; }
  inline const static QCommandLineOption port_option() { return portOption; }


};

#endif