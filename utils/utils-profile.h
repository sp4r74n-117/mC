#pragma once
#include "utils/utils-printable.h"
#include <tuple>
#include <cinttypes>
#include <map>
#include <vector>

namespace utils {
namespace profile {
  class Profiler : public Printable {
    struct Location : public std::pair<std::string, std::string> {
      using std::pair<std::string, std::string>::pair;
      const std::string& getCallee() const { return first; }
      const std::string& getCaller() const { return second; }
    };
    typedef std::vector<uint64_t> Cycles;
    const std::string& resolve(const std::string& addr);

    std::map<Location, Cycles> data;
    std::map<std::string, std::string> syms;

    std::string executable;
    std::string profile;
  public:
    Profiler(const std::string& executable, const std::string& profile);
    bool run();
    std::ostream& printTo(std::ostream& stream) const override;
  };
}
}
