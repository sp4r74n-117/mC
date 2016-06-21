#pragma once
#include <string>

namespace utils {
namespace mangle {
  std::string mangle(const std::string& id);
  std::string demangle(const std::string& id);
}
}
