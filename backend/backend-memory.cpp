#include "core/analysis/analysis.h"
#include "backend/backend-memory.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-types.h"
#include "core/arithmetic/arithmetic.h"

namespace backend {
namespace memory {
  namespace detail {
    unsigned getNumOfBytes(const core::VariablePtr& var) {
      if (!var->hasParent()) return 4;

      auto alloca = var->getParent();
      if (core::analysis::isConstant(alloca->getSize()))
        return core::arithmetic::getValue<unsigned>(alloca->getSize());
      else {
        assert(core::analysis::types::isArray(var->getType()) &&
          "dynamic alloca size is only supported for arrays!");
        // in this case it is 4 as it must be an array and this location
        // models the pointer to &arr[0]!
        return 4;
      }
    }
  }

  StackFrame::StackFrame(const core::VariableList& params, const core::VariableList& locals) :
    params(params), locals(locals) {
    numOfBytesLocals = 0;
    // determine the total number of bytes
    for (const auto& local : locals)
      numOfBytesLocals += detail::getNumOfBytes(local);
  }

  int StackFrame::getRelativeOffset(const core::VariablePtr& var) const {
    int result = getRelativeOffsetCached(var);
    if (result != 0) return result;

    // is it a parameter?
    auto it = std::find_if(params.begin(), params.end(),
      [&](const core::VariablePtr& param) { return *param == *var; });
    if (it != params.end()) {
      // care, the params are stored in-order whereas the stack layout is reverse
      auto index = std::distance(params.begin(), it);
      // 8 as we need to pass the ret-addr and index eq. 0 is first arg
      return 8 + index*4;
    }

    // try to find it in the locals list
    it = std::find_if(locals.begin(), locals.end(),
      [&](const core::VariablePtr& local) { return *local == *var; });
    assert(it != locals.end() && "cannot obtain relative offset of variable not bound to stack frame");

    std::advance(it, 1);
    // care, locals are negative in offset
    std::for_each(locals.begin(), it, [&](const core::VariablePtr& local) {
      result -= detail::getNumOfBytes(local);
    });
    cache.insert(std::make_pair(var, std::make_shared<int>(result)));
    return result;
  }

  int StackFrame::getRelativeOffset(insn::MachineOperand::Register reg) const {
    // scratch is only supported for %edx
    switch (reg) {
    case insn::MachineOperand::OR_Edx: return -(4 + getNumOfBytesLocals());
    default: break;
    }
    assert(false && "unsupported scratch register");
  }

  int StackFrame::getRelativeOffsetCached(const core::VariablePtr& var) const {
    auto it = cache.find(var);
    // as 0 is the only value that cannot be returned to the user!
    if (it == cache.end()) return 0;
    return *(it->second);
  }

  unsigned StackFrame::getNumOfBytesParameters() const {
    return params.size() * 4;
  }

  unsigned StackFrame::getNumOfBytesLocals() const {
    return numOfBytesLocals;
  }

  unsigned StackFrame::getNumOfBytesScratch() const {
    return 4;
  }

  unsigned StackFrame::getNumOfBytesFrame() const {
    return getNumOfBytesLocals() + getNumOfBytesScratch();
  }

  StackFramePtr getStackFrame(const core::FunctionPtr& fun) {
    const auto& params = fun->getParameters();
    auto locals = core::analysis::controlflow::getAllVars(fun, true);

    // remove all params from the locals set
    for (const auto& param : params)
      locals.erase(param);

    return std::make_shared<StackFrame>(params, core::VariableList(locals.begin(), locals.end()));
  }
}
}
