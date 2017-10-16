#include <string>
#include <vector>

#include "cpprofiler/utils/string_utils.hh"
#include "cpprofiler/utils/path_utils.hh"

using std::vector;
using std::string;

namespace utils {

vector<string> getPathHead(const string& path,
                           bool leaveModel,
                           bool includeTrail) {
  vector<string> pathSplit = utils::split(path, major_sep);

  if(!includeTrail && leaveModel)
      return {pathSplit.back()};

  string mzn_file;
  vector<string> previousHead;

  if(pathSplit.size() == 0)
    return previousHead;

  size_t i=0;
  do {
    string path_head = pathSplit[i];
    vector<string> head = utils::split(path_head, minor_sep);
    string head_file;
    if(head.size() > 0) {
      if(i==0) mzn_file = head[0];
      head_file = head[0];
    }

    if(!leaveModel && head_file != mzn_file)
      return previousHead;

    if(!includeTrail)
      previousHead.clear();
    previousHead.push_back(path_head);
    i++;
  } while(i < pathSplit.size());

  return previousHead;
}

}
