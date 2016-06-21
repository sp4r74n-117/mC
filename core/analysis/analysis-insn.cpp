#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis-controlflow.h"

namespace core {
namespace analysis {
namespace insn {
	bool isInsn(const NodePtr& insn) {
		return insn && insn->getNodeCategory() == Node::NC_Insn;
	}

	bool isReturnInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Return;
	}

	bool isGotoInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Goto;
	}

	bool isFalseJumpInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_FalseJump;
	}

	bool isCallInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Call;
	}

	bool isAssignInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Assign;
	}

	bool isAllocaInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Alloca;
	}

	bool hasReturnValue(const CallInsnPtr& insn) {
		return insn && insn->getResult() != nullptr;
	}

	bool hasReturnValue(const ReturnInsnPtr& insn) {
		return insn && insn->getRhs() != nullptr;
	}

	bool isPushInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Push;
	}

	bool isPopInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Pop;
	}

	bool isLoadInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Load;
	}

	bool isStoreInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_Store;
	}

	bool isPushSpInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_PushSp;
	}

	bool isPopSpInsn(const InsnPtr& insn) {
		return insn && insn->getInsnType() == Insn::IT_PopSp;
	}

	optional<LabelInsnPtr> getJumpTarget(const InsnPtr& insn) {
		if (isGotoInsn(insn)) return cast<GotoInsn>(insn)->getTarget();
		if (isFalseJumpInsn(insn)) return cast<FalseJumpInsn>(insn)->getTarget();
		return {};
	}

	optional<FunctionPtr> getCallTarget(const InsnPtr& insn) {
		if (isCallInsn(insn)) return cast<CallInsn>(insn)->getCallee();
		return {};
	}

	InsnList getSuccessors(const InsnPtr& insn) {
		InsnList result;

		auto parent = insn->getParent();
		const auto& insns = parent->getInsns();
		// find our position with our parent
		auto it = std::find(insns.begin(), insns.end(), insn);
		assert(it != insns.end() && "insn does not belong to parent");

		// are we the last one?
		if (*insns.rbegin() == insn) {
			auto succs  = controlflow::getSuccessors(parent->getParent(), parent);
			for (const auto& succ : succs) {
				const auto& insns = succ->getInsns();
				if (insns.size())	result.push_back(*std::begin(insns));
			}
		} else {
			// ok, return the one which is adjacent to us
			result.push_back(*++it);
		}
		return result;
	}

	VariableSet getInputVars(const InsnPtr& insn) {
		return getInputVars(insn, preds::all);
	}

	VariableSet getOutputVars(const InsnPtr& insn) {
		return getOutputVars(insn, preds::all);
	}
}
}
}
