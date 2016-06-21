#include "core/passes/passes.h"
#include "core/analysis/analysis.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/arithmetic/arithmetic.h"
#include <algorithm>

namespace core {
namespace passes {

	void NormalizeAssignmentsPass::apply() {
		for (const auto& fun : manager.getProgram()->getFunctions())
			apply(fun);
	}

	void NormalizeAssignmentsPass::apply(const FunctionPtr& fun) {
		for (auto bb : fun->getBasicBlocks())
			apply(bb);
	}

	void NormalizeAssignmentsPass::apply(const BasicBlockPtr& bb) {
		const auto& insns = bb->getInsns();
		// hop into the actual transformation loop
		for (auto curr = std::cbegin(insns); curr != std::cend(insns); ++curr) {
			// we search for +,- and * ops where we can move constants from right to left
			// we search for / where we can rewrite the float div into a mul
			if (!analysis::insn::isAssignInsn(*curr)) continue;

			auto assign = cast<AssignInsn>(*curr);
			if (!assign->isBinary()) continue;
			// grab both sides as we will need them anyway
			auto rhs1 = assign->getRhs1();
			auto rhs2 = assign->getRhs2();
			// does the op match?
			switch (assign->getOp()) {
			case AssignInsn::ADD:
			case AssignInsn::MUL:
					// swap them?
					if (analysis::isConstant(rhs1) && !analysis::isConstant(rhs2)) {
						assign->setRhs1(rhs2);
						assign->setRhs2(rhs1);
					}
					break;
			case AssignInsn::DIV:
					if (analysis::isFloatConstant(rhs2)) {
						// replace the whole thing with a multiplication
						assign->setOp(AssignInsn::MUL);
						assign->setRhs2(manager.buildFloatConstant(1.0f / arithmetic::getValue<float>(rhs2)));
					}
					break;
			default:
					break;
			}
		}
	}
}
}
