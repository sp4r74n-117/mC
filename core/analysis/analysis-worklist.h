#pragma once
#include <set>
#include "core/core.h"
#include "core/analysis/analysis-controlflow.h"

namespace core {
namespace analysis {
namespace worklist {

	template <typename T>
	using VarSet = PtrSet<T>;
	template <typename T, typename U>
	using Map = std::unordered_map<Ptr<T>, VarSet<U>>;

	class NodeData  : public Printable {
		VariableSet liveIn;
		VariableSet liveOut;

		public:
			const VariableSet& getLiveIn() const;
			VariableSet& getLiveIn();
			const VariableSet& getLiveOut() const;
			VariableSet& getLiveOut();
			void setLiveIn(const VariableSet& set);
			void setLiveOut(const VariableSet& set);

			std::ostream& printTo(std::ostream& stream) const override;
	};
	class NodeData;
	typedef Ptr<NodeData> NodeDataPtr;
	template<typename T>
	using NodeDataMap = std::unordered_map<Ptr<T>, NodeDataPtr>;

	enum ProblemDirection {
		PD_Forward,
		PD_Backward
	};

	template<typename T, typename U>
	class WorklistAlgorithm {
		protected:
			WorklistAlgorithm(ProblemDirection dir) : dir(dir) { }

			NodeDataPtr buildNodeData() {
				return std::make_shared<NodeData>();
			}

			typedef Ptr<U> node_type;

			virtual void init() = 0;
			virtual void calcIn(const node_type& bb) = 0;
			virtual void calcOut(const node_type& bb) = 0;
			virtual void pushResult() = 0;
			ProblemDirection dir;
			Map<U, T> in;
			Map<U, T> out;

		public:
			NodeDataMap<U>& getNodeData() { return nodeData; }
			const NodeDataMap<U>& getNodeData() const { return nodeData; }
			NodeDataMap<U> nodeData;

			template<typename ForwardIt>
			void apply(ForwardIt begin, ForwardIt end) {
				bool change = true;
				bool forward = (dir == PD_Forward);

				init();

				VarSet<T> oldIn;
				VarSet<T> oldOut;

				while(change) {
					change = false;
					std::for_each(begin, end, [&](const auto& bb) {
						if(forward) {
							oldOut = out[bb];
							this->calcIn(bb);
							this->calcOut(bb);
							change |= (oldOut != out[bb]);
						} else {
							oldIn = in[bb];
							this->calcOut(bb);
							this->calcIn(bb);
							change |= (oldIn != in[bb]);
						}
					});
				}
				pushResult();
			}
	};
}
}
namespace utils {
	std::string toString(const analysis::worklist::NodeData& nd);
}
}
