#pragma once
#include "core/passes/passes.h"

namespace core {
namespace passes {

	class NormalizeAssignmentsPass : public Pass {
	public:
		NormalizeAssignmentsPass(NodeManager& manager) :
			Pass(manager) {}
		void apply() override;
	private:
		void apply(const FunctionPtr& fun);
		void apply(const BasicBlockPtr& bb);
	};
}
}
