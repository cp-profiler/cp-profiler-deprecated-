#ifndef NOGOOD_REPRESENTATION_H
#define NOGOOD_REPRESENTATION_H

#include <string>
#include <unordered_map>
#include <cpprofiler/universal.hh>

struct NogoodViews {
  std::string original;
  std::string renamed;
  std::string simplified;

  NogoodViews() {}
  NogoodViews(std::string orig) : original(orig) {}
};

using Uid2Nogood = std::unordered_map<NodeUID, NogoodViews>;

#endif // NOGOOD_REPRESENTATION_H
