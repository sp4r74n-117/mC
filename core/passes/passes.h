#pragma once
#include "core/core.h"
#include <list>

namespace core {
namespace passes {
	class Pass;
	typedef sptr<Pass> PassPtr;

	class Pass {
	public:
		virtual ~Pass() {}
		virtual void apply() = 0;
	protected:
		NodeManager& manager;
		Pass(NodeManager& manager) : manager(manager) {}
	};

	template<typename TPass, typename... TArgs>
	PassPtr makePass(TArgs&&... args) {
		return std::make_shared<TPass>(std::forward<TArgs>(args)...);
	}

	class IntegrityPass : public Pass {
	public:
		IntegrityPass(NodeManager& manager) :
			Pass(manager) {}
		void apply() override;
	};

	class PassSequence : public Pass {
		std::vector<PassPtr> passes;
	public:
		template<typename... TPass>
		PassSequence(NodeManager& manager, TPass... passes) :
			Pass(manager), passes(toVector(passes...)) {}
		PassSequence(NodeManager& manager, const std::vector<PassPtr>& passes) :
			Pass(manager), passes(passes) {}
		void apply() override;
	};

	PassPtr makePassSequence(NodeManager& manager, bool loopAnalysis = false);
}
}

#include "core/passes/passes-lvn.h"
#include "core/passes/passes-ssa.h"
#include "core/passes/passes-inline.h"
#include "core/passes/passes-loop.h"
#include "core/passes/passes-normalize.h"
