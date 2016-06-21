#pragma once
#include "utils/utils-graph.h"

namespace utils {
namespace graph {
namespace dominator {
	template<typename TVertex>
	using DominatorSet = PtrSet<TVertex>;

	template<typename TVertex>
	using DominatorMap = std::unordered_map<typename Graph<TVertex>::vertex_type, DominatorSet<TVertex>>;

	template<typename T, typename TVertex>
	optional<TVertex> getImmediateDominator(const DominatorMap<T>& map, const TVertex& vertex) {
		auto expected = map.at(vertex);
		expected.erase(vertex);
		for (const auto& pred : expected) {
			if (map.at(pred) == expected) return pred;
		}
		return optional<TVertex>{};
	}

	template<typename TGraph, typename T = typename TGraph::vertex_raw_type, typename TVertex = typename TGraph::vertex_type>
	DominatorMap<T> getDominatorMap(const TGraph& graph) {
		bool changed = true;
		const auto& bbs = graph.getVertices();
		DominatorMap<T> result;
		// setup the initial map
		result.insert(std::make_pair(bbs[0], DominatorSet<T>({bbs[0]})));

		DominatorSet<T> all(bbs.begin(), bbs.end());
		std::for_each(bbs.begin()+1, bbs.end(), [&](const TVertex& bb) {
			result.insert(std::make_pair(bb, all));
		});
		while (changed) {
			changed = false;
			std::for_each(bbs.begin(), bbs.end(), [&](const TVertex& bb) {
				auto preds = graph.getPredecessors(bb);
				if (preds.empty()) return;

				auto fst = result[preds[0]];
				DominatorSet<T> cur(fst.begin(), fst.end());

				std::for_each(preds.begin()+1, preds.end(), [&](const TVertex& pred) {
					cur = algorithm::set_intersection(cur, result[pred]);
				});

				cur.insert(bb);
				if (result[bb] != cur) {
					result[bb] = cur;
					changed = true;
				}
			});
		}
		return result;
	}

	template<typename TGraph, typename T = typename TGraph::vertex_raw_type, typename TVertex = typename TGraph::vertex_type>
	DominatorMap<T> getDominatorFrontierMap(const TGraph& graph, const DominatorMap<T>& dominators) {
		DominatorMap<T> result;
		// increases the complexity to 2N but makes handling in real algo easier
		for (const auto& bb: graph.getVertices()) {
			result.insert(std::make_pair(bb, DominatorSet<T>{}));
		}

		for (const auto& bb: graph.getVertices()) {
			auto preds = graph.getPredecessors(bb);
			// only consider bbs which are actually a merge of branches
			if (preds.size() < 2) continue;

			auto idom = getImmediateDominator(dominators, bb);
			// omit optional check as only root has no dominator and hence preds of 0!

			for (const auto& pred: preds) {
				auto runner = pred;
				while (runner != *idom) {
					result[runner].insert(bb);

					auto next = getImmediateDominator(dominators, runner);
					if (!next) break;

					// not it is save to prepare the next iteration
					runner = *next;
				}
			}
		}
		return result;
	}
}
}
}
