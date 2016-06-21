#include "core/checks/checks.h"
#include "core/analysis/analysis.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-insn.h"
#include <list>
#include <unordered_set>

namespace core {
namespace checks {
	bool FunctionChecker::check() {
		bool success = true;
		for (const auto& fun : program->getFunctions()) {
			if (!check(fun)) success = false;
		}
		return success;
	}

	bool AssignChecker::check(const BasicBlockPtr& bb) {
		for (const auto& insn : bb->getInsns()) {
			if (insn->getInsnType() != Insn::IT_Assign) continue;

			auto assign = cast<AssignInsn>(insn);
			auto lhs = assign->getLhs();
			auto rhs1 = assign->getRhs1();
			auto rhs2 = assign->getRhs2();

			bool result = true;
			// regardless of the op, these both must always exist!
			auto lhsTypeId  = lhs->getType()->getTypeId();
			auto rhs1TypeId = rhs1->getType()->getTypeId();
			auto rhs2TypeId = assign->isBinary() ? rhs2->getType()->getTypeId() : rhs1TypeId;
			if (analysis::types::isArray(rhs1->getType())) {
				// grab the element type instead
				auto type = cast<core::ArrayType>(rhs1->getType());
				rhs1TypeId = type->getElementType()->getTypeId();
				// assure that lhs is an offset!
				result &= analysis::isOffset(lhs);
				// hack: rhs2 is supposed to act as rhs1Type
				rhs2TypeId = rhs1TypeId;
			}

			if (assign->isBinary())
				result &= rhs1TypeId == rhs2TypeId;

			if (AssignInsn::isLogicalBinaryOp(assign->getOp()))
				result &= lhsTypeId == Type::TI_Int;
			else
				result &= lhsTypeId == rhs1TypeId;
			// also make sure that it is never void
			result &= lhsTypeId != Type::TI_Void;
			if (!result) {
				assign->printTo(std::cerr);
				std::cerr << std::endl;
				return false;
			}
		}
		return true;
	}

	std::string AssignChecker::getName() const {
		return "AssignChecker";
	}

	bool AssignChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!check(bb)) return false;
		return true;
	}

	bool LoadChecker::check(const BasicBlockPtr& bb) {
		for (const auto& insn : bb->getInsns()) {
			if (!analysis::insn::isLoadInsn(insn)) continue;

			auto load = cast<LoadInsn>(insn);
			if (!analysis::isMemory(load->getSource()) &&
			    !analysis::isOffset(load->getSource())) return false;
		}
		return true;
	}

	std::string LoadChecker::getName() const {
		return "LoadChecker";
	}

	bool LoadChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!check(bb)) return false;
		return true;
	}

	bool StoreChecker::check(const BasicBlockPtr& bb) {
		for (const auto& insn : bb->getInsns()) {
			if (!analysis::insn::isStoreInsn(insn)) continue;

			auto store = cast<StoreInsn>(insn);
			if (!analysis::isMemory(store->getTarget()) &&
			    !analysis::isOffset(store->getTarget())) return false;
		}
		return true;
	}

	std::string StoreChecker::getName() const {
		return "StoreChecker";
	}

	bool StoreChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!check(bb)) return false;
		return true;
	}

	bool GotoChecker::check(const FunctionPtr& fun, const BasicBlockPtr& bb) {
		for (const auto& insn : bb->getInsns()) {
			if (insn->getInsnType() != Insn::IT_Goto) continue;

			std::string target = cast<GotoInsn>(insn)->getTarget()->getName();
			auto result = analysis::controlflow::findBasicBlock(fun, [&](const BasicBlockPtr& bb) {
				return bb->getLabel()->getName() == target;
			});
			if (!result) {
				insn->printTo(std::cerr);
				std::cerr << std::endl;
				return false;
			}
		}
		return true;
	}

	std::string GotoChecker::getName() const {
		return "GotoChecker";
	}

	bool GotoChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!check(fun, bb)) return false;
		return true;
	}

	bool FalseJumpChecker::check(const FunctionPtr& fun, const BasicBlockPtr& bb) {
		for (const auto& insn : bb->getInsns()) {
			if (insn->getInsnType() != Insn::IT_FalseJump) continue;

			auto jumpInsn = cast<FalseJumpInsn>(insn);
			auto target = jumpInsn->getTarget();
			auto result = analysis::controlflow::findBasicBlock(fun, [&](const BasicBlockPtr& bb) {
				return *(bb->getLabel()) == *target;
			});
			if (!result || jumpInsn->getCond()->getType()->getTypeId() != Type::TI_Int) {
				insn->printTo(std::cerr);
				std::cerr << std::endl;
				return false;
			}
		}
		return true;
	}

	std::string FalseJumpChecker::getName() const {
		return "FalseJumpChecker";
	}

	bool FalseJumpChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!check(fun, bb)) return false;
		return true;
	}

	bool CallChecker::check(const FunctionPtr& fun, const BasicBlockPtr& bb) {
		const auto& insns = bb->getInsns();
		for (auto it = insns.cbegin(); it != insns.cend(); ++it) {
			if (!analysis::insn::isCallInsn(*it)) continue;

			auto call = cast<CallInsn>(*it);
			auto type = call->getCallee()->getType();
			if (type->getParameterTypes().size()) {
				// construct a reverse iterator which lets us process the pushes/pops
				auto rit = insns.crbegin();
				std::advance(rit, std::distance(it, insns.cend()));

				TypeList actualTypes;
				for (unsigned ign = 0, arg = 0, nargs = type->getParameterTypes().size(); rit != insns.crend() && arg < nargs; ++rit) {
					auto insn = *rit;
					if (analysis::insn::isPopInsn(insn)) {
						// adjust the number of ignored pushes, this is important for nested calls
						ign += cast<PopInsn>(insn)->getNumOfBytes() / 4;
					} else if (analysis::insn::isPushInsn(insn)) {
						// shall we ignore this one?
						if (ign) { --ign; continue; }
						// check the type of this push
						auto push = cast<PushInsn>(insn);
						// remember this type for later checks
						actualTypes.push_back(push->getRhs()->getType());
						++arg;
					}
				}

				if (!analysis::types::isCallable(type, actualTypes)) {
					std::cerr << "malformed function call, invalid argument types" << std::endl;
					return false;
				}
			}

			// also check if we assign a from a void fun
			if (analysis::insn::hasReturnValue(call) != analysis::types::hasReturn(type)) {
				std::cerr << "malformed function call, cannot assign void to register" << std::endl;
				return false;
			}

			// also check if the return types match
			if (analysis::insn::hasReturnValue(call) &&
				(*call->getResult()->getType() != *analysis::types::getReturnType(type))) {
				std::cerr << "malformed function call, cannot assign to register from different type" << std::endl;
				return false;
			}
		}
		return true;
	}

	std::string CallChecker::getName() const {
		return "CallChecker";
	}

	bool CallChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!check(fun, bb)) return false;
		return true;
	}

	bool ReturnChecker::check(const FunctionPtr& fun, const BasicBlockPtr& bb) {
		const auto& insns = bb->getInsns();
		for (auto it = insns.cbegin(); it != insns.cend(); ++it) {
			if (!analysis::insn::isReturnInsn(*it)) continue;

			auto ret = cast<ReturnInsn>(*it);
			auto type = analysis::types::getFunctionType(fun->getType());
			if (analysis::insn::hasReturnValue(ret) != analysis::types::hasReturn(type)) {
				std::cerr << "malformed function exit, either return or function are void" << std::endl;
				return false;
			}

			// also check if the return types match
			if (analysis::insn::hasReturnValue(ret) &&
				(*ret->getRhs()->getType() != *analysis::types::getReturnType(type))) {
				std::cerr << "malformed function exit, cannot return a value from different type" << std::endl;
				return false;
			}
		}
		return true;
	}

	std::string ReturnChecker::getName() const {
		return "ReturnChecker";
	}

	bool ReturnChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!check(fun, bb)) return false;
		return true;
	}

	std::string LabelChecker::getName() const {
		return "LabelChecker";
	}

	bool LabelChecker::check() {
		std::unordered_set<std::string> labels;
		for (const auto& fun : program->getFunctions()) {
			for (const auto& bb : fun->getBasicBlocks()) {
				std::string target = bb->getLabel()->getName();
				if (!labels.insert(target).second) {
					std::cerr << "duplicated bb label " << target << std::endl;
					return false;
				}
			}
		}
		return true;
	}

	std::string BasicBlockChecker::getName() const {
		return "BasicBlockChecker";
	}

	bool BasicBlockChecker::check(const FunctionPtr& fun) {
		for (const auto& bb : fun->getBasicBlocks())
			if (!bb->isValid()) return false;
		return true;
	}

	std::string EdgeChecker::getName() const {
		return "EdgeChecker";
	}

	bool EdgeChecker::check(const FunctionPtr& fun) {
		// if the function is external, there is nothing to check
		if (core::analysis::callgraph::isExternalFunction(fun)) return true;
		// bb which has no incoming edges
		unsigned startPoints = 0;
		// bb which has no outgoing edges
		unsigned endPoints = 0;

		for (const auto& bb : fun->getBasicBlocks()) {
			auto ei = analysis::controlflow::getEdges(fun, bb, Direction::IN);
			auto eo = analysis::controlflow::getEdges(fun, bb, Direction::OUT);

			if (ei.empty()) ++startPoints;
			if (eo.empty()) {
				// this is only valid under three conditions:
				// 1. no insns available
				// 2. last insn is a goto
				// 3. last insn is a return
				// 4. anonymous function
				if (bb->getInsns().size() == 0 ||
						analysis::callgraph::isAnonymousFunction(fun) ||
					  analysis::insn::isGotoInsn(*bb->getInsns().rbegin()) ||
						analysis::insn::isReturnInsn(*bb->getInsns().rbegin())) {
					++endPoints;
				} else {
					// whoops, this not not allowed!
					return false;
				}
			}
			// this is an absolute no-go!
			if (eo.size() > 2) return false;
		}
		return (startPoints == 1) && (endPoints >= 1);
	}

	bool fullCheck(const ProgramPtr& program) {
		std::list<CheckerPtr> checkers;
		checkers.push_back(std::make_shared<AssignChecker>(program));
		checkers.push_back(std::make_shared<GotoChecker>(program));
		checkers.push_back(std::make_shared<LabelChecker>(program));
		checkers.push_back(std::make_shared<FalseJumpChecker>(program));
		checkers.push_back(std::make_shared<CallChecker>(program));
		checkers.push_back(std::make_shared<ReturnChecker>(program));
		checkers.push_back(std::make_shared<LoadChecker>(program));
		checkers.push_back(std::make_shared<StoreChecker>(program));
		checkers.push_back(std::make_shared<BasicBlockChecker>(program));
		checkers.push_back(std::make_shared<EdgeChecker>(program));

		bool fail = false;
		for (const auto& checker : checkers) {
			bool result = checker->check();
			if (!result)
				std::cerr << " check " << checker->getName() << " has failed!" << std::endl;
			fail |= !result;
		}
		return !fail;
	}
}
}
