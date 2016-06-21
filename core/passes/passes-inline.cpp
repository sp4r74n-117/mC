#include "core/passes/passes.h"
#include "core/analysis/analysis.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/arithmetic/arithmetic.h"
#include <algorithm>

namespace core {
namespace passes {

	void InlineAssignmentsPass::apply() {
		for (const auto& fun : manager.getProgram()->getFunctions())
			apply(fun);
	}

	void InlineAssignmentsPass::apply(const FunctionPtr& fun) {
		for (auto bb : fun->getBasicBlocks())
			apply(bb);
	}

	void InlineAssignmentsPass::apply(const BasicBlockPtr& bb) {
		// we search for patterns like:
		// $N = rhs
		// vN = $N
		// and transform them into:
		// vN = rhs
		const auto& insns = bb->getInsns();
		auto transform = [&](InsnList::const_iterator& curr) -> bool {
			if (!analysis::insn::isAssignInsn(*curr)) return false;
			// take a look at the next one
			auto next = std::next(curr);
			// in this case we have reached the end and no more inlining can occur
			if (next == std::cend(insns)) return false;
			// it must be an assignment as well
			if (!analysis::insn::isAssignInsn(*next)) return false;

			auto ac = cast<AssignInsn>(*curr);
			auto an = cast<AssignInsn>(*next);
			// also assure that is a plain assignment
			if (!an->isAssign()) return false;
			// assure that it is not an offset
			if (analysis::isOffset(ac->getLhs())) return false;
			// is in and output criteria met?
			if (*ac->getLhs() != *an->getRhs1()) return false;
			// excellent do the inline of assign
			ac->setLhs(an->getLhs());
			curr = BasicBlock::remove(bb, next);
			return true;
		};
		// hop into the actual transformation loop
		for (auto curr = std::cbegin(insns); curr != std::cend(insns);) {
			// in case transform returns true, the iterator has changed already!
			if (!transform(curr)) std::advance(curr, 1);
		}
	}
}
}
