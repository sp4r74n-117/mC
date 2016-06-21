#pragma once
#include "core/core.h"
#include "core/analysis/analysis-worklist.h"

namespace core {
namespace analysis {
namespace worklist {
	class BasicBlockLiveness : public WorklistAlgorithm<Variable, BasicBlock> {
		FunctionPtr fun;
		public:
			BasicBlockLiveness(const FunctionPtr& fun) : WorklistAlgorithm (PD_Backward), fun(fun) {}
		private:
			typedef WorklistAlgorithm<Variable, BasicBlock>::node_type node_type;
			void init() override;
			void calcIn(const node_type& bb) override;
			void calcOut(const node_type& bb) override;
			void pushResult() override;
	};

	class InsnLiveness : public WorklistAlgorithm<Variable, Insn> {
		public:
			InsnLiveness() : WorklistAlgorithm (PD_Backward) {}
		private:
			typedef WorklistAlgorithm<Variable, Insn>::node_type node_type;
			void init() override;
			void calcIn(const node_type& bb) override;
			void calcOut(const node_type& bb) override;
			void pushResult() override;
	};
}
}
}
