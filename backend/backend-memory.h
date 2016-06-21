#pragma once

#include "core/core.h"
#include "backend/backend-insn.h"

namespace backend {
namespace memory {
  /**
   * Models variables and their associated stack location
   * high address:
   *
   * arg N
   * ...
   * arg 1
   * arg 0
   * ret addr
   * old ebp <-- ebp points here
   * local vars ...
   *
   * low address:
   */
  class StackFrame {
    core::VariableList params;
    core::VariableList locals;
    unsigned numOfBytesLocals;
    mutable PtrMap<core::Variable, int> cache;
    int getRelativeOffsetCached(const core::VariablePtr& var) const;
  public:
    StackFrame(const core::VariableList& params, const core::VariableList& locals);
    const core::VariableList& getParameters() const { return params; }
    const core::VariableList& getLocals() const { return locals; }
    unsigned getNumOfBytesParameters() const;
    unsigned getNumOfBytesLocals() const;
    unsigned getNumOfBytesScratch() const;
    unsigned getNumOfBytesFrame() const;
    int getRelativeOffset(const core::VariablePtr& var) const;
    int getRelativeOffset(insn::MachineOperand::Register reg) const;
  };
  typedef Ptr<StackFrame> StackFramePtr;

  StackFramePtr getStackFrame(const core::FunctionPtr& fun);
}
}
