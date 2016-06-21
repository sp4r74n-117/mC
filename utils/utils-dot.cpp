#include "utils/utils-dot.h"
#include <cstdlib>
#include <algorithm>

namespace utils {
namespace dot {
  namespace {
    bool endsWith(const std::string& value, const std::string& ending) {
        if (ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }
  }

  bool generatePng(const std::string& dotFile) {
    // check if it proposes the right format
    if (!endsWith(dotFile, ".dot")) return false;
    // put together the output filename
    std::string dstFile = dotFile;
    dstFile.replace(dotFile.length()-3, 3, "png");
    return std::system(std::string("dot -Tpng -o \"" + dstFile + "\" \"" + dotFile + "\"").data()) == 0;
  }
}
}
