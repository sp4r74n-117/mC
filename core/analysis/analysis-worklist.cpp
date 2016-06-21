#include "core/analysis/analysis-worklist.h"

namespace core {
namespace analysis {
namespace worklist {
  const VariableSet& NodeData::getLiveIn() const {
		return liveIn;
	}

	VariableSet& NodeData::getLiveIn() {
		return liveIn;
	}

	const VariableSet& NodeData::getLiveOut() const {
		return liveOut;
	}

	VariableSet& NodeData::getLiveOut() {
		return liveOut;
	}

	void NodeData::setLiveIn(const VariableSet& set) {
		liveIn = set;
	}

	void NodeData::setLiveOut(const VariableSet& set) {
		liveOut = set;
	}

	std::ostream& NodeData::printTo(std::ostream& stream) const {
		stream << "LIVEIN: {";
		bool first = true;
		for(auto in : liveIn){
			if(first) {
				in->printTo(stream);
				first = false;
			} else {
				stream << ", ";
				in->printTo(stream);
			}
		}

		stream << "}" << std::endl << "LIVEOUT: {";
		first = true;

		for(auto out : liveOut){
			if(first) {
				out->printTo(stream);
				first = false;
			} else {
				stream << ", ";
				out->printTo(stream);
			}
		}
		stream << "}";
		return stream;
	}
}
}
namespace utils {
	std::string toString(const analysis::worklist::NodeData& nd) {
    std::stringstream ss;
		nd.printTo(ss);
		return ss.str();
  }
}
}
