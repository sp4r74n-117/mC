#pragma once
#include "backend/backend-insn.h"

namespace backend {
namespace instrument {
  insn::TemplateInsnPtr buildInstrumentationEntryTemplate();
  insn::TemplateInsnPtr buildInstrumentationLeaveTemplate();
}
}
