#pragma once
#include "core/passes/passes.h"
#include <functional>

namespace core {
namespace passes {

	class SSAEncoderPass : public Pass {
		void apply(const FunctionPtr& fun);
	public:
		using Pass::Pass;
		void apply() override;
	};

	class SSADecoderPass : public Pass {
	public:
		using Pass::Pass;
		void apply() override;
	};
}
}
