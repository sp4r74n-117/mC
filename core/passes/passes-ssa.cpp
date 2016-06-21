#include "core/passes/passes.h"
#include "core/analysis/analysis.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-insn.h"
#include <algorithm>

namespace core {
namespace passes {

	void SSAEncoderPass::apply(const FunctionPtr& fun) {
		// compute which bbs have an assignment to vars first
		std::unordered_map<BasicBlockPtr, VariableSet> modifiedVars;
		for (const auto& bb: fun->getBasicBlocks()) {
			modifiedVars.insert(std::make_pair(bb, analysis::controlflow::getModifiedVars(bb)));
		}

		// compute the dominator & its associated frontier
		auto dominators = analysis::controlflow::getDominatorMap(fun);
		auto frontier = analysis::controlflow::getDominatorFrontierMap(fun, dominators);

		typedef analysis::controlflow::DominatorSet WorkList;
		// iterate over all vars for phi introduction
		for (const auto& var : analysis::controlflow::getAllVars(fun)) {
			WorkList hasAlready;
			WorkList workList;

			// do this vor each node X containing an assignment to V
			for (const auto& bb: fun->getBasicBlocks()) {
				auto& value = modifiedVars[bb];
				auto it = value.find(var);
				// nope, it's not there
				if (it == value.end()) continue;

				workList.insert(bb);
			}

			// this loop will terminate if all phi insn for V have been created
			while (!workList.empty()) {
				// get the _first_ node within the list and remove it afterwards
				auto it = workList.begin();
				auto cur = *it;
				workList.erase(it);

				for (const auto& bb: frontier[cur]) {
					auto it = hasAlready.find(bb);
					// in this case we need to process bb
					if (it == hasAlready.end()) {
						bool found = false;
						auto idom = analysis::controlflow::getImmediateDominator(dominators, bb);
						auto runner = *idom;
						while (!found) {
							auto& value = modifiedVars[runner];
							auto it = value.find(var);
							// nope, it's not there
							if (it != value.end()) {
								found = true;
								break;
							}

							auto next = analysis::controlflow::getImmediateDominator(dominators, runner);
							if (!next) break;

							// not it is save to prepare the next iteration
							runner = *next;
						}

						if (found) {
							VariableList rhs;
							for (unsigned i = 0; i < analysis::controlflow::getPredecessors(fun, bb).size(); ++i)
								rhs.push_back(var);

								BasicBlock::prepend(bb, manager.buildPhi(var, rhs));
						}

						hasAlready.insert(bb);
						workList.insert(bb);
					}
				}
			}
		}
	}

	void SSAEncoderPass::apply() {
		for (const auto& fun : manager.getProgram()->getFunctions())
			apply(fun);
	}

	void SSADecoderPass::apply() {
		/* no-op */
	}
}
}
