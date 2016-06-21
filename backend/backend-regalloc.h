#pragma once
#include "backend/backend.h"
#include "backend/backend-memory.h"
#include "backend/backend-insn.h"
#include "utils/utils-graph-color.h"

namespace backend {
namespace regalloc {
  class RegAllocContext;
  typedef Ptr<RegAllocContext> RegAllocContextPtr;

  class RegAllocMatcher;
  typedef Ptr<RegAllocMatcher> RegAllocMatcherPtr;

  class RegAllocBackend : public Backend {
    std::string targetCode;
    RegAllocContextPtr context;
  public:
    RegAllocBackend(const core::ProgramPtr& program);
    bool convert() override;
    std::ostream& printTo(std::ostream& stream) const override;
    const RegAllocContextPtr& getContext() const { return context; }
  private:
    bool convert(std::stringstream& ss, const core::FunctionPtr& fun);
    std::vector<RegAllocMatcherPtr> matchers;
  };
}
}
