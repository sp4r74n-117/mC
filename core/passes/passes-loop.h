#pragma once
#include "core/passes/passes.h"

namespace core {
namespace passes {
	class LoopAnalysisPass : public Pass {
	public:
		LoopAnalysisPass(NodeManager& manager) :
			Pass(manager) {}
		void apply() override;
	private:
		void apply(const FunctionPtr& fun);
	};
}
}
