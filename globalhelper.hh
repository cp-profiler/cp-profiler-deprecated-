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

#ifndef GLOBALHELPER_HH
#define GLOBALHELPER_HH

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QString>
#include <memory>
#include <chrono>
#include <QDebug>

class QCoreApplication;

class GlobalParser {
 private:
  static GlobalParser* _self;

  static QCommandLineParser clParser;

 public:
  static QCommandLineOption help_option;
  static QCommandLineOption version_option;

  static QCommandLineOption test_option;
  static QCommandLineOption port_option;
  static QCommandLineOption load_option;
  static QCommandLineOption save_log;
  static QCommandLineOption auto_compare;

  static QCommandLineOption auto_stats;

 public:
  GlobalParser();
  ~GlobalParser();

  static bool isSet(const QCommandLineOption& opt);

  static QString value(const QCommandLineOption& opt);

  static void process(const QCoreApplication& app);
};


class Utils {
public:
  /// query the filename with a system dialog and fill it with `str`
  static void writeToFile(const QString& str);

  /// write str to the file at path
  static void writeToFile(const QString& path, const QString& str);

};

class Settings {
  static QJsonObject json_setttings;
public:
  static void init();
  static int get_int(QString name);
  static bool get_bool(QString name);
};

#include <type_traits>
#include <utility>

template <typename T, typename... Args>
std::unique_ptr<T> make_unique_helper(std::false_type, Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
std::unique_ptr<T> make_unique_helper(std::true_type, Args&&... args) {
   static_assert(std::extent<T>::value == 0,
       "make_unique<T[N]>() is forbidden, please use make_unique<T[]>().");

   typedef typename std::remove_extent<T>::type U;
   return std::unique_ptr<T>(new U[sizeof...(Args)]{std::forward<Args>(args)...});
}

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
   return make_unique_helper<T>(std::is_array<T>(), std::forward<Args>(args)...);
}

/// Helper function for the one below
template<typename T>
static size_t findAnyOf(const std::string& str, T el) {
  return str.find(el);
}

/// Return the position of some substrings from args in str,
/// starting from the end of the list
template <typename T, typename... Delimiters>
static size_t findAnyOf(const std::string& str, T first, Delimiters... args) {

  auto pos = findAnyOf(str, args...);

  if (pos != std::string::npos) return pos;

  pos = findAnyOf(str, first);

  return pos;
}

#endif
