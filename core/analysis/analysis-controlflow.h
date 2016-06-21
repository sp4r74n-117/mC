#pragma once
#include "core/core.h"
#include <unordered_map>
#include <type_traits>
#include <set>

namespace core {
namespace analysis {
namespace controlflow {
	template<typename Lambda>
	BasicBlockPtr findBasicBlock(const FunctionPtr& function, Lambda lambda) {
		auto result = function->getGraph().findVertex(lambda);
		if (!result) return {};
		else			   return *result;
	}

	template<typename Lambda>
	BasicBlockList findBasicBlocks(const FunctionPtr& function, Lambda lambda) {
		return function->getGraph().findVertices(lambda);
	}

	EdgeList getEdges(const FunctionPtr& function, const BasicBlockPtr& bb, Direction dir);

	BasicBlockPtr getEntryPoint(const FunctionPtr& function);
	BasicBlockPtr getExitPoint(const FunctionPtr& function);

	BasicBlockList getPredecessors(const FunctionPtr& function, const BasicBlockPtr& bb);
	BasicBlockList getSuccessors(const FunctionPtr& function, const BasicBlockPtr& bb);

	typedef PtrSet<BasicBlock> DominatorSet;
	typedef std::unordered_map<BasicBlockPtr, DominatorSet> DominatorMap;
	DominatorMap getDominatorMap(const FunctionPtr& function);

	optional<BasicBlockPtr> getImmediateDominator(const DominatorMap& map, const BasicBlockPtr& bb);

	DominatorMap getDominatorFrontierMap(const FunctionPtr& function, const DominatorMap& dominators);

	std::vector<BasicBlockList> getExtendedBasicBlocks(const FunctionPtr& function);

	VariableSet getIncomingVars(const BasicBlockPtr& bb, bool visitTemporaries = false);

	VariableSet getModifiedVars(const BasicBlockPtr& bb, bool visitTemporaries = false);

	VariableSet getAllVars(const BasicBlockPtr& bb, bool visitTemporaries = false);

	VariableSet getAllVars(const FunctionPtr& function, bool visitTemporaries = false);

	BasicBlockList getLinearBasicBlockList(const FunctionPtr& fun);

	InsnList getLinearInsnList(const FunctionPtr& fun);
	InsnList getLinearInsnList(const BasicBlockList& bbs);

	class ControFlowPrinter : public GraphPrinter<BasicBlock, directed> {
	public:
		ControFlowPrinter(const Function& function) :
			GraphPrinter(function.getGraph())
		{}
	protected:
		std::string getGraphLabel() const override;
		std::string getVertexId(const vertex_type& vertex) const override;
		std::string getVertexLabel(const vertex_type& vertex) const override;
		std::string getEdgeLabel(const edge_type& edge) const override;
	};
}
}
}
namespace utils {
	std::string toString(const core::analysis::controlflow::DominatorSet& ds);
	std::string toString(const core::analysis::controlflow::ControFlowPrinter& gv);
}
