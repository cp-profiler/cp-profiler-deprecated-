#include "maybeCaller.hh"
#include <iostream>


MaybeCaller::MaybeCaller() {
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
  }
}