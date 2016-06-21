#include "core/passes/passes.h"
#include "core/analysis/analysis-loop.h"
#include "core/analysis/analysis-callgraph.h"

namespace core {
namespace passes {
	void LoopAnalysisPass::apply() {
		for (const auto& fun : manager.getProgram()->getFunctions()) {
			if (analysis::callgraph::isExternalFunction(fun)) continue;
			apply(fun);
		}
	}

	namespace detail {
		struct StatementLess {
				bool operator()(const analysis::loop::StatementPtr& fst,
					const analysis::loop::StatementPtr& snd) const {
						return *fst->getLocation() < *snd->getLocation();
				}
		};

		void checkDependency(NodeManager& manager, const analysis::loop::StatementPtr& stmt1,
			const analysis::loop::SubscriptPtr& sub1, const analysis::loop::StatementPtr& stmt2,
			const analysis::loop::SubscriptPtr& sub2) {
			if (sub1 == sub2) return;
			if (*sub1->getVariable() != *sub2->getVariable()) return;
			if (sub2->getType() == analysis::loop::Subscript::UNKNOWN) return;
			// now check for conflict
			bool result = analysis::loop::hasNoDependency(manager, sub1, sub2);

			std::string type;
			// determine the dependency type
			if (result)
				type = "\033[0;32mno dependency\033[0m";
			else if (sub1->getType() == sub2->getType())
				type = "\033[0;31m(?) output dependency\033[0m";
			else if (*stmt1->getLocation() < *stmt2->getLocation())
				type = "\033[0;31m(?) true dependency\033[0m";
			else
				type = "\033[0;31m(?) anti dependency\033[0m";

			stmt1->getLocation()->printTo(std::cout);
			sub1->printTo(std::cout << " ");
			std::cout << " has " << type << " with ";
			stmt2->getLocation()->printTo(std::cout);
			sub2->printTo(std::cout << " ");
			std::cout << std::endl;
		}

		void checkDependency(NodeManager& manager, const analysis::loop::LoopPtr& loop,
			analysis::loop::StatementList& parent) {
			analysis::loop::StatementList derived = parent;
			derived.insert(derived.end(), loop->getStatements().begin(), loop->getStatements().end());
			std::sort(derived.begin(), derived.end(), StatementLess());
			// hop into our children
			for (const auto& child : loop->getChildren())
				checkDependency(manager, child, derived);
			// check with our statements
			for (const auto& stmt1: loop->getStatements()) {
				for (const auto& sub1: stmt1->getSubscripts()) {
					// skip all which are not a write
					if (sub1->getType() != analysis::loop::Subscript::WRITE) continue;
					for (const auto& stmt2: derived) {
						for (const auto& sub2: stmt2->getSubscripts()) {
							checkDependency(manager, stmt1, sub1, stmt2, sub2);
						}
					}
				}
			}
		}
	}

	void LoopAnalysisPass::apply(const FunctionPtr& fun) {
		auto loops = analysis::loop::findLoops(manager, fun);
		// parent list is always empty
		analysis::loop::StatementList parent;
    for (const auto& loop : loops)
			detail::checkDependency(manager, loop, parent);
  }
}
}
