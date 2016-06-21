#include "utils/utils-mangle.h"

namespace utils {
namespace mangle {
  std::string mangle(const std::string& id) {
		return "_" + id;
	}

  std::string demangle(const std::string& id) {
    if (id.empty()) return id;
    if (id.at(0) == '_') return id.substr(1);
    return id;
  }
}
}
