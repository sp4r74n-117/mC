#include "backend/backend-instrument.h"

namespace backend {
namespace instrument {
  static unsigned uniqueId = 0;

  namespace detail {
    insn::MachineOperandPtr buildInstrumentLabel(unsigned id) {
      return insn::buildLocOperand("__cyg_profile_eip" + std::to_string(id));
    }

    void buildInstrumentCall(insn::MachineInsnList& insns, const std::string& callee) {
	    // push call_site (caller is return addr is located at 4(%ebp)!)
      appendAll(insns, insn::buildPushTemplate(insn::buildMemOperand(4))->getInsns());
      // push this_fn, eip is with bounds rest does not matter, however use a little trick :)
      insns.push_back(insn::buildCallInsn(buildInstrumentLabel(uniqueId)));
      insns.push_back(insn::buildLabelInsn(buildInstrumentLabel(uniqueId++)));
      insns.push_back(insn::buildCallInsn(insn::buildLocOperand(callee)));
      // fixup stack
      appendAll(insns, insn::buildPopTemplate(insn::buildImmOperand(8))->getInsns());
	  }
  }

  insn::TemplateInsnPtr buildInstrumentationEntryTemplate() {
    insn::MachineInsnList insns;
    // generate a call for:
    // void __cyg_profile_func_enter(void *this_fn, void *call_site)
    detail::buildInstrumentCall(insns, "__cyg_profile_func_enter");
    return std::make_shared<insn::TemplateInsn>(insns);
  }

  insn::TemplateInsnPtr buildInstrumentationLeaveTemplate() {
    insn::MachineInsnList insns;
    // generate a call for:
    // void __cyg_profile_func_exit(void *this_fn, void *call_site)
    detail::buildInstrumentCall(insns, "__cyg_profile_func_exit");
    return std::make_shared<insn::TemplateInsn>(insns);
  }
}
}
