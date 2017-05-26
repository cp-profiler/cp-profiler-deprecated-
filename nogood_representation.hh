#ifndef NOGOOD_REPRESENTATION_H
#define NOGOOD_REPRESENTATION_H

#include <string>
#include <unordered_map>

struct NogoodViews {
  std::string original;
  std::string renamed;
  std::string simplified;

  NogoodViews() {}
  NogoodViews(std::string orig) : original(orig) {}
};

using Sid2Nogood = std::unordered_map<int64_t, NogoodViews>;

#endif // NOGOOD_REPRESENTATION_H
