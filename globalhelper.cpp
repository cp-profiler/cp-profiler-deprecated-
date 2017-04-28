/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "globalhelper.hh"
#include <iostream>
#include <sstream>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>

GlobalParser* GlobalParser::_self = nullptr;

QCommandLineParser GlobalParser::clParser{};
QCommandLineOption GlobalParser::help_option{clParser.addHelpOption()};

QCommandLineOption GlobalParser::version_option{clParser.addVersionOption()};

QCommandLineOption GlobalParser::test_option{
    "test", "Terminates right after getting DONE RECEIVING"};

QCommandLineOption GlobalParser::port_option{
    {"p", "port"}, "Send nodes via port <port>.", "port"};

QCommandLineOption GlobalParser::load_option{
    {"l", "load"}, "Load execution <file_name>.", "file_name"};

QCommandLineOption GlobalParser::paths_option{
    "paths", "Use symbol table from: <file_name>.", "file_name"};

QCommandLineOption GlobalParser::mzn_option{
    "mzn", "Use MiniZinc file for tying ids to expressions: <file_name>.", "file_name"};

QCommandLineOption GlobalParser::save_log{
    "save_log", "Save search log to <file_name>."};

QCommandLineOption GlobalParser::auto_compare{
    "auto_compare", "Compare the first two executions"};

QCommandLineOption GlobalParser::auto_stats{
    "auto_stats", "Write statistics to <file_name>.", "file_name"};

GlobalParser::GlobalParser() {
  if (_self) {
    std::cerr << "Can't have two of GlobalParser, terminate\n";
    abort();
  }
  _self = this;

  port_option.setDefaultValue("6565");

  clParser.addOption(test_option);
  clParser.addOption(port_option);
  clParser.addOption(load_option);
  clParser.addOption(mzn_option);
  clParser.addOption(paths_option);
  clParser.addOption(save_log);
  clParser.addOption(auto_compare);
  clParser.addOption(auto_stats);
}

bool GlobalParser::isSet(const QCommandLineOption& opt) {
  return _self->clParser.isSet(opt);
}

void GlobalParser::process(const QCoreApplication& app) {
  _self->clParser.process(app);
}

QString GlobalParser::value(const QCommandLineOption& opt) {
  return _self->clParser.value(opt);
}

GlobalParser::~GlobalParser() {}

void Utils::writeToFile(const QString& str) {
  QString file_name =
      QFileDialog::getSaveFileName(nullptr, "Save File", "");

  if (file_name == "") return;

  writeToFile(file_name, str);
}

void Utils::writeToFile(const QString& path, const QString& str) {
  QFile file(path);

  if (file.open(QFile::WriteOnly | QFile::Truncate)) {
    QTextStream out(&file);

    out << str;

  } else {
    qDebug() << "could not open the file: " << path;
  }
}

static QJsonObject readJson(QString path) {
  QFile file(path);
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QString json_str = file.readAll();
  
  QJsonDocument json_doc = QJsonDocument::fromJson(json_str.toUtf8());
  QJsonObject json_obj = json_doc.object();

  return json_obj;
}

void Settings::init() {
  // ofstream json_file("settings.json");
  // QJsonObject json

  json_setttings = readJson("settings.json");


}

QJsonObject Settings::json_setttings = {};

int Settings::get_int(QString name) {
  return json_setttings.value(name).toInt();
}

bool Settings::get_bool(QString name) {
  return json_setttings.value(name).toBool();
}
