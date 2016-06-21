#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis.h"
#include "utils/utils-graph-dominator.h"
#include <algorithm>

namespace core {
namespace analysis {
namespace controlflow {
	BasicBlockPtr getEntryPoint(const FunctionPtr& function) {
		return findBasicBlock(function, [&](const BasicBlockPtr& bb) {
			return getPredecessors(function, bb).empty();
		});
	}

	BasicBlockPtr getExitPoint(const FunctionPtr& function) {
		return findBasicBlock(function, [&](const BasicBlockPtr& bb) {
			return getSuccessors(function, bb).empty();
		});
	}

	EdgeList getEdges(const FunctionPtr& function, const BasicBlockPtr& bb, Direction dir) {
		return function->getGraph().getConnectedEdges(bb, dir);
	}

	BasicBlockList getPredecessors(const FunctionPtr& function, const BasicBlockPtr& bb) {
		return function->getGraph().getPredecessors(bb);
	}

	BasicBlockList getSuccessors(const FunctionPtr& function, const BasicBlockPtr& bb) {
		return function->getGraph().getSuccessors(bb);
	}

	DominatorMap getDominatorMap(const FunctionPtr& function) {
		return graph::dominator::getDominatorMap(function->getGraph());
	}

	optional<BasicBlockPtr> getImmediateDominator(const DominatorMap& map, const BasicBlockPtr& bb) {
		return graph::dominator::getImmediateDominator(map, bb);
	}

	DominatorMap getDominatorFrontierMap(const FunctionPtr& function, const DominatorMap& dominators) {
		return graph::dominator::getDominatorFrontierMap(function->getGraph(), dominators);
	}

	namespace {
		void collectExtendedBasicBlocks(const FunctionPtr& function, const BasicBlockList& leaders,
										const BasicBlockPtr& bb, BasicBlockList& result) {
			// check for loops
			auto it = std::find(result.begin(), result.end(), bb);
			if (it != result.end()) return;
			// check for end of ebb -- skip if result is empty as this is the base-case
			if (result.size()) {
				auto it = std::find(leaders.begin(), leaders.end(), bb);
				if (it != leaders.end()) return;
			}
			// remember this one
			result.push_back(bb);
			// and collect all children -- recursion will stop if ebb is complete
			for (const auto& succ : controlflow::getSuccessors(function, bb))
				collectExtendedBasicBlocks(function, leaders, succ, result);
		}
	}

	std::vector<BasicBlockList> getExtendedBasicBlocks(const FunctionPtr& function) {
		std::vector<BasicBlockList> result;
		// first step is to find all leaders of ebbs
		auto leaders = controlflow::findBasicBlocks(function, [&](const BasicBlockPtr& bb) {
			auto preds = controlflow::getPredecessors(function, bb);
			// in case of root
			if (preds.empty()) return true;
			// in case we have multiple preds
			if (preds.size() > 1) return true;
			return false;
		});
		// second step is to expand each leader to the ebb set
		for (const auto& leader : leaders) {
			result.push_back(BasicBlockList{});
			collectExtendedBasicBlocks(function, leaders, leader, result.back());
		}
		return result;
	}

	std::string ControFlowPrinter::getGraphLabel() const {
		return "cfg";
	}

	std::string ControFlowPrinter::getVertexId(const vertex_type& vertex) const {
		return vertex->getLabel()->getName();
	}

	std::string ControFlowPrinter::getVertexLabel(const vertex_type& vertex) const {
		std::stringstream ss;
		for (const auto& insn : vertex->getInsns()) {
			if (ss.tellp()) ss << "\\n";
			insn->printTo(ss);
		}
		return ss.str();
	}

	std::string ControFlowPrinter::getEdgeLabel(const edge_type& edge) const {
		return getVertexId(edge->getSource()) + " -> " + getVertexId(edge->getTarget());
	}

	VariableSet getIncomingVars(const BasicBlockPtr& bb, bool visitTemporaries) {
		VariableSet result;
		VariableSet defined;

		auto pushVar = [&](const core::ValuePtr& val) {
			if (!val) return;

			auto var = cast<Variable>(val);
			if (!visitTemporaries && isTemporary(var)) return;

			auto it = defined.find(var);
			if (it != defined.end()) return;
			// ok unseen so push it into the result
			result.insert(var);
		};
		for (const auto& insn : bb->getInsns()) {
			auto vi = insn::getInputVars(insn, visitTemporaries ? insn::preds::all : insn::preds::mem);
			auto vo = insn::getOutputVars(insn, visitTemporaries ? insn::preds::all : insn::preds::mem);

			for (const auto& var: vi)	pushVar(var);
			// add to defined set
			defined.insert(vo.begin(), vo.end());
		}
		return result;
	}

	VariableSet getModifiedVars(const BasicBlockPtr& bb, bool visitTemporaries) {
		VariableSet result;
		for (const auto& insn : bb->getInsns()) {
			auto vo = insn::getOutputVars(insn, visitTemporaries ? insn::preds::all : insn::preds::mem);
			result.insert(vo.begin(), vo.end());
		}
		return result;
	}

	namespace {
		void collectVariables(const BasicBlockPtr& bb, bool visitTemporaries, VariableSet& result) {
			for (const auto& insn : bb->getInsns()) {
				auto vi = insn::getInputVars(insn, visitTemporaries ? insn::preds::all : insn::preds::mem);
				auto vo = insn::getOutputVars(insn, visitTemporaries ? insn::preds::all : insn::preds::mem);

				result.insert(vi.begin(), vi.end());
				result.insert(vo.begin(), vo.end());
			}
		}
	}

	VariableSet getAllVars(const BasicBlockPtr& bb, bool visitTemporaries) {
		VariableSet result;
		collectVariables(bb, visitTemporaries, result);
		return result;
	}

	VariableSet getAllVars(const FunctionPtr& function, bool visitTemporaries) {
		VariableSet result;
		for (const auto& bb : function->getBasicBlocks())
			collectVariables(bb, visitTemporaries, result);
		return result;
	}

	namespace {
    void collectBasicBlocks(BasicBlockList& result, const FunctionPtr& fun, const BasicBlockPtr& current) {
      auto it = std::find_if(result.begin(), result.end(),
        [&](const BasicBlockPtr& bb) { return *bb == *current; });
      if (it != result.end()) return;

      // insert the current block to the result
      result.push_back(current);

      auto succs = getSuccessors(fun, current);
      // find the immediate one, there must exist one (expect we are at the end)
      if (succs.empty()) return;

      // in case we are empty, advance
      if (current->getInsns().empty()) {
        assert(succs.size() == 1 && "malformed BB graph");
        collectBasicBlocks(result, fun, succs[0]);
        return;
      }

      auto other = insn::getJumpTarget(*current->getInsns().rbegin());
      if (succs.size() == 1) {
        if (!other) collectBasicBlocks(result, fun, succs[0]);
      } else {
        bool condition = *succs[0]->getLabel() == **other;
        collectBasicBlocks(result, fun, condition ? succs[1] : succs[0]);
        collectBasicBlocks(result, fun, condition ? succs[0] : succs[1]);
      }
    }
  }

  BasicBlockList getLinearBasicBlockList(const FunctionPtr& fun) {
    BasicBlockList result;
		if (callgraph::isExternalFunction(fun)) return result;
    collectBasicBlocks(result, fun, getEntryPoint(fun));
    return result;
  }

	InsnList getLinearInsnList(const core::FunctionPtr& fun) {
		return getLinearInsnList(getLinearBasicBlockList(fun));
	}

	InsnList getLinearInsnList(const BasicBlockList& bbs) {
		InsnList insns;
		for (const auto& bb : bbs) {
			for (const auto& insn : bb->getInsns())
				insns.push_back(insn);
		}
		return insns;
	}
}
}
}
namespace utils {
	std::string toString(const core::analysis::controlflow::DominatorSet& ds) {
		std::stringstream ss;
		ss << "{";
		bool first = true;
		for (const auto& bb: ds) {
			if (first) first = false;
			else	   ss << ",";
			ss << bb->getLabel()->getName();
		}
		ss << "}";
		return ss.str();
	}

	std::string toString(const core::analysis::controlflow::ControFlowPrinter& gv) {
		std::stringstream ss;
		gv.printTo(ss);
		return ss.str();
	}
}
