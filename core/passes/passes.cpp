#include "passes.h"
#include "core/checks/checks.h"

namespace core {
namespace passes {
	void IntegrityPass::apply() {
		assert(checks::fullCheck(manager.getProgram()) && "integrity pass has failed");
	}

	void PassSequence::apply() {
		for (auto pass : passes) pass->apply();
	}

	PassPtr makePassSequence(NodeManager& manager, bool loopAnalysis) {
		std::vector<PassPtr> passes;
		passes.push_back(makePass<InlineAssignmentsPass>(manager));
		if (loopAnalysis)	passes.push_back(makePass<LoopAnalysisPass>(manager));
		passes.push_back(makePass<NormalizeAssignmentsPass>(manager));
		// do not swap with previous ones, as it would introduce errors to the code
		passes.push_back(makePass<SuperLocalValueNumberingPass>(manager));
		passes.push_back(makePass<IntegrityPass>(manager));
		return makePass<PassSequence>(manager, passes);
	}
}
}
