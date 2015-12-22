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

#include "maybeCaller.hh"
#include <iostream>

MaybeCaller::MaybeCaller() {
  updateTimer.setSingleShot(true);
  connect(&updateTimer, SIGNAL(timeout()), this, SLOT(callViaTimer()));
  last_call_time = std::chrono::system_clock::now();
}

void
MaybeCaller::call(std::function<void (void)> fn) {
  using namespace std::chrono;

  auto now = system_clock::now();
  auto elapsed = duration_cast<milliseconds>(now - last_call_time).count();

  if (elapsed > MIN_ELAPSED) {
    fn();
    last_call_time = system_clock::now();
  } else {
    /// call delayed
    delayed_fn = fn;
    updateTimer.start(MIN_ELAPSED);
  }
}

void
MaybeCaller::callViaTimer() {
  delayed_fn();
}
