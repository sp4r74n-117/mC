#pragma once
#include "core/core.h"
#include "core/analysis/analysis.h"

namespace core {
namespace analysis {
namespace insn {
	bool isInsn(const NodePtr& insn);
	bool isReturnInsn(const InsnPtr& insn);
	bool isGotoInsn(const InsnPtr& insn);
	bool isFalseJumpInsn(const InsnPtr& insn);
	bool isCallInsn(const InsnPtr& insn);
	bool isPushInsn(const InsnPtr& insn);
	bool isPopInsn(const InsnPtr& insn);
	bool isAssignInsn(const InsnPtr& insn);
	bool isAllocaInsn(const InsnPtr& insn);
	bool isLoadInsn(const InsnPtr& insn);
	bool isStoreInsn(const InsnPtr& insn);
	bool isPushSpInsn(const InsnPtr& insn);
	bool isPopSpInsn(const InsnPtr& insn);
	bool hasReturnValue(const CallInsnPtr& insn);
	bool hasReturnValue(const ReturnInsnPtr& insn);

	optional<LabelInsnPtr> getJumpTarget(const InsnPtr& insn);
	optional<FunctionPtr> getCallTarget(const InsnPtr& insn);

	namespace preds {
		static std::function<bool(const VariablePtr&)> all = [](const VariablePtr& var) { return true; };
		static std::function<bool(const VariablePtr&)> mem = [](const VariablePtr& var) { return analysis::isMemory(var); };
		static std::function<bool(const VariablePtr&)> tmp = [](const VariablePtr& var) { return analysis::isTemporary(var); };

		template<typename Lambda>
		void insertIf(const VariablePtr& var, Lambda lambda, VariableSet& output) {
			if (var && lambda(var)) output.insert(var);
		}
	}

	template<typename Lambda>
	VariableSet getInputVars(const InsnPtr& insn, Lambda pred) {
		VariableSet result;
		switch (insn->getInsnType()) {
		case Insn::IT_Assign:
			{
				auto assign = cast<AssignInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(assign->getRhs1()), pred, result);
				preds::insertIf(dyn_cast<Variable>(assign->getRhs2()), pred, result);
			}
			break;
		case Insn::IT_Phi:
			{
				auto phi = cast<PhiInsn>(insn);
				for (const auto& rhs : phi->getRhs())
					preds::insertIf(rhs, pred, result);
			}
			break;
		case Insn::IT_Push:
			{
				auto push = cast<PushInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(push->getRhs()), pred, result);
			}
			break;
		case Insn::IT_PushSp:
			{
				auto push = cast<PushSpInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(push->getRhs()), pred, result);
			}
			break;
		case Insn::IT_PopSp:
			{
				auto pop = cast<PopSpInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(pop->getRhs()), pred, result);
			}
			break;
		case Insn::IT_Return:
			{
				auto ret = cast<ReturnInsn>(insn);
				if (hasReturnValue(ret)) preds::insertIf(dyn_cast<Variable>(ret->getRhs()), pred, result);
			}
			break;
		case Insn::IT_Load:
			{
				auto load = cast<LoadInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(load->getSource()), pred, result);
			}
			break;
		case Insn::IT_Store:
			{
				auto store = cast<StoreInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(store->getSource()), pred, result);
				preds::insertIf(dyn_cast<Variable>(store->getTarget()), pred, result);
			}
		case Insn::IT_Alloca:
			{
				auto alloca = cast<AllocaInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(alloca->getSize()), pred, result);
			}
			break;
		default: break;
		}
		return result;
	}

	VariableSet getInputVars(const InsnPtr& insn);

	template<typename Lambda>
	VariableSet getOutputVars(const InsnPtr& insn, Lambda pred) {
		VariableSet result;
		switch (insn->getInsnType()) {
		case Insn::IT_Assign:
			{
				auto assign = cast<AssignInsn>(insn);
				preds::insertIf(assign->getLhs(), pred, result);
			}
			break;
		case Insn::IT_Phi:
			{
				auto phi = cast<PhiInsn>(insn);
				preds::insertIf(phi->getLhs(), pred, result);
			}
			break;
		case Insn::IT_Pop:
			{
				auto pop = cast<PopInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(pop->getRhs()), pred, result);
			}
			break;
		case Insn::IT_PushSp:
			{
				auto push = cast<PushSpInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(push->getRhs()), pred, result);
			}
			break;
		case Insn::IT_Call:
			{
				auto call = cast<CallInsn>(insn);
				if (hasReturnValue(call)) preds::insertIf(call->getResult(), pred, result);
			}
			break;
		case Insn::IT_Load:
			{
				auto load = cast<LoadInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(load->getTarget()), pred, result);
			}
			break;
		case Insn::IT_Store:
			{
				auto store = cast<StoreInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(store->getTarget()), pred, result);
			}
			break;
		case Insn::IT_Alloca:
		  {
				auto alloca = cast<AllocaInsn>(insn);
				preds::insertIf(dyn_cast<Variable>(alloca->getVariable()), pred, result);
		  }
			break;
		default: break;
		}
		return result;
	}

	VariableSet getOutputVars(const InsnPtr& insn);
	InsnList getSuccessors(const InsnPtr& insn);
}
}
}
