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
 */

#include "globalhelper.hh"
#include "cpprofiler/tests/tests.hh"
#include "profiler-conductor.hh"
#include <QApplication>

int main(int argc, char *argv[]) {
#ifdef QT_OPENGL_SUPPORT
  QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
#endif

  QApplication a(argc, argv);

  QCoreApplication::setApplicationName("CP-Profiler");
  QCoreApplication::setApplicationVersion("0.2");

  GlobalParser clParser;

  clParser.process(a);

  Settings::init();

  if (GlobalParser::isSet(GlobalParser::version_option) ||
      GlobalParser::isSet(GlobalParser::help_option)) {
    return 0;
  }

  if (GlobalParser::isSet(GlobalParser::unit_test)) {
    cpprofiler::unit_tests::run();
    return 0;
  }

  ProfilerConductor conductor;

  conductor.show();

  std::string mzn_name;
  if (GlobalParser::isSet(GlobalParser::mzn_option)) {
    mzn_name = GlobalParser::value(GlobalParser::mzn_option).toStdString();
    qDebug() << "loading mzn file: " << QString::fromStdString(mzn_name);
  }

  if (GlobalParser::isSet(GlobalParser::paths_option)) {
    std::string file_name = GlobalParser::value(GlobalParser::paths_option).toStdString();
    qDebug() << "loading paths file: " << QString::fromStdString(file_name);
    conductor.getNextExecId("", file_name, NameMap(file_name, mzn_name));
  }

  /// NOTE(maxim): only can load 1 execution for now
  if (GlobalParser::isSet(GlobalParser::load_option)) {
    auto file_name = GlobalParser::value(GlobalParser::load_option);
    qDebug() << "loading execuiton: " << file_name;
    conductor.loadExecution(file_name.toStdString());
  }

  return a.exec();
}
