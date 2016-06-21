#pragma once
#include "utils/utils-graph.h"

namespace utils {
namespace graph {
namespace color {
	template<typename TVertex>
	using ColorGraph = UndirectedGraph<TVertex>;

	template<typename TVertex, typename TColor = int>
	struct Mapping {
		typename ColorGraph<TVertex>::vertex_type vertex;
		TColor color; // -1 if not mapping could be found
		bool flag; // indicates whether or not the associated vertex was removed randomly
	};
	template<typename TVertex, typename TColor = int>
	using Mappings = std::vector<Mapping<TVertex, TColor>>;
	// graph coloring problem can be solved as follows:
	// 1. Pick any node |n|<k and put on stack
	// 2. Remove that node and its edges -this reduces degree of neighbors
	// 3. Any remaining nodes -spill one and continue
	// 4. Pop nodes off stack and color
	template<typename TVertex, typename TColor = int, typename TResult = Mappings<TVertex, TColor>>
	TResult getColorMappings(const ColorGraph<TVertex>& graph, unsigned numberOfColors) {
		// represents the stack, however the coloring itself is done two phases:
		// 1. populate the stack in the right order (stack.push == vector.push_back)
		// 2. colorize afterwards
		TResult result;
		if (!numberOfColors) return result;
		// obtain a local copy of the graph as we need to remove vertices
		ColorGraph<TVertex> problem = graph;
		while (!problem.empty()) {
			// in case we traverse through all vertices, automatically remember the
			// one which we will remove as next
			unsigned maxNumberOfNeighbours = numberOfColors;
			typename ColorGraph<TVertex>::vertex_type fallback = *std::begin(problem.getVertices());
			// now do the actual traversal
			auto res = problem.findVertex([&](const typename ColorGraph<TVertex>::vertex_type& vertex) {
				auto numberOfNeighbours = problem.getConnectedEdges(vertex).size();
				// is this one our potential next fallback candidate?
				if (numberOfNeighbours > maxNumberOfNeighbours) {
					maxNumberOfNeighbours = numberOfNeighbours;
					// also save a reference to this vertex
					fallback = vertex;
					// save the additional comparison, we already know that it is above!
					return false;
				}
				// |v| < k ?
				return numberOfNeighbours < numberOfColors;
			});
			bool flag = false;
			typename ColorGraph<TVertex>::vertex_type v;
			// did we find a vertex v where |v|<k
			if (res) { v = *res; }
			// select one with the most connected edges and indicate so by flag
			else { v = fallback; flag = true; }
			// -1 indicates 'dont know yet'
			result.push_back({v, -1, flag});
			problem.removeVertex(v);
		}
		// colorize the result -- simulate 'pop' by traversing reverse
		for (auto it = std::rbegin(result); it != std::rend(result); std::advance(it, 1)) {
			typename ColorGraph<TVertex>::vertex_equal_type cmp;
			// hold a vector of used colors, basically it is a vector of bits where
			// each bits represents a color index
			std::vector<bool> used(numberOfColors);
			auto neighbours = graph.getConnectedVertices(it->vertex);
			for (const auto& neighbour : neighbours) {
				// check those we have already mapped
				std::for_each(std::rbegin(result), it, [&](const Mapping<TVertex, TColor>& mapping) {
					if (cmp(mapping.vertex, neighbour) && mapping.color >= 0) used[mapping.color] = true;
				});
			}
			// find a new color assignment
			unsigned i = 0;
			for (; used[i] && i < used.size(); ++i);
			it->color = i < used.size() ? i : -1;
		}
		return result;
	}

	template<typename TVertex, typename TColor = int>
	class ColorGraphPrinter : public GraphPrinter<TVertex, undirected> {
		const Mappings<TVertex, TColor>& mappings;
	public:
		typedef typename ColorGraph<TVertex>::vertex_type vertex_type;
		typedef typename ColorGraph<TVertex>::edge_type edge_type;
		ColorGraphPrinter(const ColorGraph<TVertex>& graph, const Mappings<TVertex, TColor>& mappings) :
			GraphPrinter<TVertex, undirected>(graph), mappings(mappings) {}
	protected:
		std::string getGraphLabel() const override { return "colorgraph"; }
		std::string getVertexId(const vertex_type& vertex) const override { return toString(*vertex);	}
		std::string getVertexLabel(const vertex_type& vertex) const override { return getVertexId(vertex); }
		std::string getEdgeLabel(const edge_type& edge) const override { return ""; }
		std::string getVertexAttributes(const vertex_type& vertex) const override {
			typename ColorGraph<TVertex>::vertex_equal_type cmp;
			auto it = std::find_if(std::begin(mappings), std::end(mappings),
				[&](const Mapping<TVertex, TColor>& mapping) { return cmp(vertex, mapping.vertex); });
			std::string color = "white";
			if (it != std::end(mappings)) {
				switch (it->color) {
				case 0: color = "red"; break;
				case 1: color = "green"; break;
				case 2: color = "yellow"; break;
				case 3: color = "blue"; break;
				case 4: color = "magenta"; break;
				case 5: color = "cyan"; break;
				default: break;
				}
			}
			return ",style=filled, fillcolor="+color;
		}
	};
}
}
}
