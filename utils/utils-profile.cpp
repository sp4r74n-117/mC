#include "utils/utils-profile.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iterator>
#include <iomanip>

namespace utils {
namespace profile {
  namespace {
    static const std::string unknown = "??";
  }

  Profiler::Profiler(const std::string& executable, const std::string& profile) :
    executable(executable), profile(profile) {}

  const std::string& Profiler::resolve(const std::string& addr) {
    auto it = syms.find(addr);
    if (it != syms.end()) return it->second;

    std::FILE* file = popen(std::string("addr2line -f -e " + executable + " " + addr).data(), "r");
    if (!file) return unknown;

    char buffer[512];
    if (!std::fgets(buffer, sizeof(buffer), file)) {
      pclose(file);
      return unknown;
    }
    // regular close of the stream
    pclose(file);
    syms.insert(std::make_pair(addr, std::string(buffer)));
    return syms[addr];
  }

  bool Profiler::run() {
    std::ifstream input{profile};
    if (!input) {
      std::cout << "cannot read file: " << profile << std::endl;
      return false;
    }

    std::stringstream ss;
    ss << input.rdbuf();
    // grab the content of the file
    std::istream_iterator<std::string> beg(ss), end;
    std::vector<std::string> tokens(beg, end);
    if ((tokens.size() % 3) != 0) {
      std::cout << "malformed file: " << profile << std::endl;
      return false;
    }

    for (unsigned i = 0; i < tokens.size(); i += 3) {
      const auto& callee = resolve(tokens[i]);
      const auto& caller = resolve(tokens[i+1]);
      uint64_t cycles = std::atoll(tokens[i+2].data());

      data[Location(callee, caller)].push_back(cycles);
    }
    return true;
  }

  std::ostream& Profiler::printTo(std::ostream& stream) const {
    for (const auto& pair : data) {
      const auto& k = pair.first;
      const auto& v = pair.second;

      double sum = std::accumulate(v.begin(), v.end(), 0.0);
      double mean = sum / v.size();

      std::vector<double> diff(v.size());
      std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
      double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      double stddev = std::sqrt(sq_sum / v.size());

      stream << std::fixed << std::setprecision(0) << "function " << k.getCallee() << " called from " << k.getCaller()
        << " took avg: " << mean << " stddev: " << stddev << " cycles" << std::endl;
    }
    return stream;
  }
}
}
