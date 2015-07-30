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


GlobalParser* GlobalParser::_self = nullptr;


QCommandLineParser GlobalParser::clParser{};
QCommandLineOption GlobalParser
      ::helpOption{clParser.addHelpOption()};

QCommandLineOption GlobalParser
      ::versionOption{clParser.addVersionOption()};

QCommandLineOption GlobalParser
      ::testOption{"test", "Terminates right after getting DONE RECEIVING"};

QCommandLineOption GlobalParser
      ::portOption{{"p", "port"}, "Send nodes via port <port>.", "port"};


GlobalParser::GlobalParser()
{

  if (_self) {
    std::cerr << "Can't have two of GlobalParser, terminate\n";
    abort();
  }
  _self = this;

  portOption.setDefaultValue("6565");

  clParser.addOption(testOption);
  clParser.addOption(portOption);


}

bool
GlobalParser::isSet(const QCommandLineOption& opt) {
  return _self->clParser.isSet(opt);
}

void
GlobalParser::process(const QCoreApplication & app) {
  _self->clParser.process(app);
}

QString
GlobalParser::value(const QCommandLineOption& opt) {
  return _self->clParser.value(opt);
}

GlobalParser::~GlobalParser() {

}