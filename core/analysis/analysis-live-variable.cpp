#include "core/analysis/analysis-live-variable.h"
#include "core/analysis/analysis-insn.h"

namespace core {
namespace analysis {
namespace worklist {
	void BasicBlockLiveness::init() {
		VarSet<Variable> init;
		for(auto bb : fun->getBasicBlocks())
			in[bb] = init;
	}

	void BasicBlockLiveness::calcIn(const node_type& bb) {
		auto use = controlflow::getIncomingVars(bb, true);
		auto def = controlflow::getModifiedVars(bb, true);
		in[bb] = algorithm::set_union(use, algorithm::set_difference(out[bb], def));
	}

	void BasicBlockLiveness::calcOut(const node_type& bb) {
		auto succs = controlflow::getSuccessors(fun, bb);
		VarSet<Variable> result;

		for(auto succ : succs)
			result = algorithm::set_union(result, in[succ]);

		out[bb] = result;
	}

	void BasicBlockLiveness::pushResult() {
		const auto& bbs = fun->getBasicBlocks();
		auto& nodeData = getNodeData();

		for(const auto& bb : bbs) {
			if(nodeData.find(bb) == nodeData.end()) {
				auto data = buildNodeData();
				nodeData.insert(std::make_pair(bb, data));
			}
			nodeData[bb]->setLiveIn(in[bb]);
			nodeData[bb]->setLiveOut(out[bb]);
		}
	}

	void InsnLiveness::init() {
		// no-op
	}

	void InsnLiveness::calcIn(const node_type& bb) {
		auto use = insn::getInputVars(bb);
		auto def = insn::getOutputVars(bb);

		in[bb] = algorithm::set_union(use, algorithm::set_difference(out[bb], def));
	}

	void InsnLiveness::calcOut(const node_type& bb) {
		auto succs = insn::getSuccessors(bb);
		VarSet<Variable> result;

		for(auto succ : succs)
			result = algorithm::set_union(result, in[succ]);

		out[bb] = result;
	}

	void InsnLiveness::pushResult() {
		auto& nodeData = getNodeData();
		for (const auto& pair : in) {
			auto key = pair.first;
			if(nodeData.find(key) == nodeData.end()) {
				auto data = buildNodeData();
				nodeData.insert(std::make_pair(key, data));
			}
			nodeData[key]->setLiveIn(pair.second);
			nodeData[key]->setLiveOut(out[key]);
		}
	}
}
}
}
