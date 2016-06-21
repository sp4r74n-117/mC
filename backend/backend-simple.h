#pragma once
#include "backend/backend-regalloc.h"

namespace backend {
namespace simple {
  class SimpleBackend : public regalloc::RegAllocBackend {
  public:
    SimpleBackend(const core::ProgramPtr& program) :
      RegAllocBackend(program) {
      setRegAlloc(false);
    }
  };
}
}
