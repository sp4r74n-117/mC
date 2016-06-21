#pragma once
#include "utils/utils-printable.h"
#include "utils/utils-container.h"
#include <unordered_map>

namespace utils {
	struct directed {};
	struct undirected {};

	enum class Direction { IN, OUT, ANY };

	template<typename TVertex, typename TDirection = directed>
	class Graph;

	namespace detail {
		template<typename TVertex, typename TDirection>
		class GraphBase;

		template<typename TVertex, typename TDirection>
		class EdgeBase {
		public:
			typedef typename GraphBase<TVertex, TDirection>::vertex_type vertex_type;
			typedef TDirection direction_type;

			EdgeBase() {}
			EdgeBase(const vertex_type& src, const vertex_type& dst) : src(src), dst(dst) {}
			const vertex_type& getSource() const { return src; }
			const vertex_type& getTarget() const { return dst; }
			void setSource(const vertex_type& src) { this->src = src; }
			void setTarget(const vertex_type& dst) { this->dst = dst; }
		private:
			vertex_type src;
			vertex_type dst;
		};
	}

	template<typename TVertex, typename TDirection>
	class Edge : public detail::EdgeBase<TVertex, TDirection> {};

	template<typename TVertex>
	class Edge<TVertex, directed> : public detail::EdgeBase<TVertex, directed> {
	public:
		using detail::EdgeBase<TVertex, directed>::EdgeBase;

		bool operator ==(const Edge<TVertex, directed>& other) const {
			typename Graph<TVertex, directed>::vertex_equal_type cmp;
			return cmp(this->getSource(), other.getSource()) && cmp(this->getTarget(), other.getTarget());
		}

		bool operator !=(const Edge<TVertex, directed>& other) const {
			return !(*this == other);
		}
	};
	template<typename TVertex>
	using DirectedEdge = Edge<TVertex, directed>;

	template<typename TVertex>
	class Edge<TVertex, undirected> : public detail::EdgeBase<TVertex, undirected> {
	public:
		using detail::EdgeBase<TVertex, undirected>::EdgeBase;

		bool operator ==(const Edge<TVertex, undirected>& other) const {
			typename Graph<TVertex, undirected>::vertex_equal_type cmp;
			return (cmp(this->getSource(), other.getSource()) && cmp(this->getTarget(), other.getTarget())) ||
						 (cmp(this->getSource(), other.getTarget()) && cmp(this->getTarget(), other.getSource()));
		}

		bool operator !=(const Edge<TVertex, undirected>& other) const {
			return !(*this == other);
		}
	};
	template<typename TVertex>
	using UndirectedEdge = Edge<TVertex, undirected>;

	template<typename TVertex, typename... TArgs>
	typename detail::GraphBase<TVertex, undirected>::vertex_type makeVertex(TArgs&&... args) {
		return std::make_shared<TVertex>(std::forward<TArgs>(args)...);
	}

	template<typename TVertex, typename TDirection, typename... TArgs>
	typename detail::GraphBase<TVertex, TDirection>::edge_type makeEdge(TArgs&&... args) {
		return std::make_shared<Edge<TVertex, TDirection>>(std::forward<TArgs>(args)...);
	}

	namespace detail {
		template<typename TVertex, typename TDirection>
		class GraphBase {
		public:
			typedef TVertex vertex_raw_type;
			typedef Ptr<vertex_raw_type> vertex_type;
			typedef PtrList<TVertex> vertex_list_type;
			typedef target_equal<TVertex> vertex_equal_type;
			typedef Edge<TVertex, TDirection> edge_raw_type;
			typedef Ptr<edge_raw_type> edge_type;
			typedef PtrList<edge_raw_type> edge_list_type;
			typedef target_equal<edge_raw_type> edge_equal_type;

			bool addVertex(const vertex_type& vertex);
			bool addEdge(const vertex_type& source, const vertex_type& target);
			bool removeVertex(const vertex_type& vertex);

			size_t numberOfVertices() const;
			size_t numberOfEdges() const;
			bool empty() const;

			template<typename TLambda>
			optional<vertex_type> findVertex(TLambda lambda) const;
			template<typename TLambda>
			vertex_list_type findVertices(TLambda lambda) const;
			template<typename TLambda>
			optional<edge_type> findEdge(TLambda lambda) const;
			template<typename TLambda>
			edge_list_type findEdges(TLambda lambda) const;

			edge_list_type& getEdges() { return edges; }
			const edge_list_type& getEdges() const { return edges; }
			vertex_list_type& getVertices() { return vertices; }
			const vertex_list_type& getVertices() const { return vertices; }
		private:
			edge_list_type edges;
			vertex_list_type vertices;
		};

		template<typename TVertex, typename TDirection>
		bool GraphBase<TVertex, TDirection>::addVertex(const vertex_type& vertex) {
			vertex_equal_type cmp;

			auto it = std::find_if(std::begin(vertices), std::end(vertices),
				[&](const vertex_type& e) { return cmp(vertex, e); });
			// dont insert twice
			if (it != std::end(vertices)) return false;

			vertices.push_back(vertex);
			return true;
		}

		template<typename TVertex, typename TDirection>
		bool GraphBase<TVertex, TDirection>::addEdge(const vertex_type& source, const vertex_type& target) {
			edge_equal_type cmp;

			addVertex(source);
			addVertex(target);

			auto edge = makeEdge<TVertex, TDirection>(source, target);
			auto it = std::find_if(std::begin(edges), std::end(edges),
				[&](const edge_type& e) { return cmp(edge, e); });
			// dont insert twice
			if (it != std::end(edges)) return false;

			edges.push_back(edge);
			return true;
		}

		template<typename TVertex, typename TDirection>
		bool GraphBase<TVertex, TDirection>::removeVertex(const vertex_type& vertex) {
			vertex_equal_type cmp;
			for (auto it = std::begin(getEdges()); it != std::end(getEdges());) {
				auto edge = *it;
				if (cmp(edge->getSource(), vertex) || cmp(edge->getTarget(), vertex))
					it = edges.erase(it);
				else
					std::advance(it, 1);
			}
			auto it = std::find_if(std::begin(getVertices()), std::end(getVertices()),
				[&](const vertex_type& element) { return cmp(element, vertex); });
			if (it == std::end(getVertices())) return false;
			getVertices().erase(it);
			return true;
		}

		template<typename TVertex, typename TDirection>
		size_t GraphBase<TVertex, TDirection>::numberOfVertices() const {
			return getVertices().size();
		}

		template<typename TVertex, typename TDirection>
		size_t GraphBase<TVertex, TDirection>::numberOfEdges() const {
			return getEdges().size();
		}

		template<typename TVertex, typename TDirection>
		bool GraphBase<TVertex, TDirection>::empty() const {
			return numberOfVertices() == 0;
		}

		template<typename TVertex, typename TDirection>
		template<typename TLambda>
		optional<typename GraphBase<TVertex, TDirection>::vertex_type> GraphBase<TVertex, TDirection>::findVertex(TLambda lambda) const {
			for (const auto& vertex : getVertices()) {
				if (lambda(vertex)) return vertex;
			}
			return {};
		}

		template<typename TVertex, typename TDirection>
		template<typename TLambda>
		typename GraphBase<TVertex, TDirection>::vertex_list_type GraphBase<TVertex, TDirection>::findVertices(TLambda lambda) const {
			vertex_list_type result;
			for (const auto& vertex : getVertices()) {
				if (lambda(vertex)) result.push_back(vertex);
			}
			return result;
		}

		template<typename TVertex, typename TDirection>
		template<typename TLambda>
		optional<typename GraphBase<TVertex, TDirection>::edge_type> GraphBase<TVertex, TDirection>::findEdge(TLambda lambda) const {
			for (const auto& edge : getEdges()) {
				if (lambda(edge)) return edge;
			}
			return {};
		}

		template<typename TVertex, typename TDirection>
		template<typename TLambda>
		typename GraphBase<TVertex, TDirection>::edge_list_type GraphBase<TVertex, TDirection>::findEdges(TLambda lambda) const {
			edge_list_type result;
			for (const auto& edge : getEdges()) {
				if (lambda(edge)) result.push_back(edge);
			}
			return result;
		}

		template<typename TVertex, typename TDirection, typename TGraph = Graph<TVertex, TDirection>>
		bool equals_unordered(const typename TGraph::vertex_list_type& fst, const typename TGraph::vertex_list_type& snd) {
			if (fst.size() != snd.size()) return false;
			typename TGraph::vertex_equal_type cmp;
			for (const auto& vertex : fst) {
				auto it = std::find_if(std::cbegin(snd), std::cend(snd),
					[&](const typename TGraph::vertex_type& other) { return cmp(vertex, other); });
				if (it == std::cend(snd)) return false;
			}
			return true;
		}

		template<typename TVertex, typename TDirection, typename TGraph = Graph<TVertex, TDirection>>
		bool equals_unordered(const typename TGraph::edge_list_type& fst, const typename TGraph::edge_list_type& snd) {
			if (fst.size() != snd.size()) return false;
			typename TGraph::edge_equal_type cmp;
			for (const auto& edge : fst) {
				auto it = std::find_if(std::cbegin(snd), std::cend(snd),
					[&](const typename TGraph::edge_type& other) { return cmp(edge, other); });
				if (it == std::cend(snd)) return false;
			}
			return true;
		}
	}

	template<typename TVertex, typename TDirection>
	class Graph : public detail::GraphBase<TVertex, TDirection> {};

	template<typename TVertex>
	class Graph<TVertex, directed> : public detail::GraphBase<TVertex, directed> {
	public:
		typedef typename detail::GraphBase<TVertex, directed> graph_type;

		typename graph_type::edge_list_type getConnectedEdges(const typename graph_type::vertex_type& vertex, Direction dir) const {
			typename graph_type::vertex_equal_type cmp;
			typename graph_type::edge_list_type result;
			for (const auto& edge : this->getEdges()) {
				if ((dir == Direction::IN || dir == Direction::ANY) && cmp(edge->getTarget(), vertex)) {
					result.push_back(edge);
					continue;
				}

				if ((dir == Direction::OUT || dir == Direction::ANY) && cmp(edge->getSource(), vertex)) {
					result.push_back(edge);
					continue;
				}
			}
			return result;
		}

		typename graph_type::vertex_list_type getPredecessors(const typename graph_type::vertex_type& vertex) const {
			typename graph_type::vertex_list_type result;
			auto edges = getConnectedEdges(vertex, Direction::IN);
			for (const auto& edge : edges)
				result.push_back(edge->getSource());
			return result;
		}

		typename graph_type::vertex_list_type getSuccessors(const typename graph_type::vertex_type& vertex) const {
			typename graph_type::vertex_list_type result;
			auto edges = getConnectedEdges(vertex, Direction::OUT);
			for (const auto& edge : edges)
				result.push_back(edge->getTarget());
			return result;
		}

		bool operator ==(const Graph<TVertex, directed>& other) const {
			return detail::equals_unordered<TVertex, directed>(this->getVertices(), other.getVertices()) &&
						 detail::equals_unordered<TVertex, directed>(this->getEdges(), other.getEdges());
		}

		bool operator !=(const Graph<TVertex, directed>& other) const {
			return !(*this == other);
		}
	};
	template<typename TVertex>
	using DirectedGraph = Graph<TVertex, directed>;

	template<typename TVertex>
	class Graph<TVertex, undirected> : public detail::GraphBase<TVertex, undirected> {
	public:
		typedef typename detail::GraphBase<TVertex, undirected> graph_type;

		typename graph_type::edge_list_type getConnectedEdges(const typename graph_type::vertex_type& vertex) const {
			typename graph_type::vertex_equal_type cmp;
			return this->findEdges([&](const typename graph_type::edge_type& edge) {
				return cmp(edge->getSource(), vertex) || cmp(edge->getTarget(), vertex);
			});
		}

		typename graph_type::vertex_list_type getConnectedVertices(const typename graph_type::vertex_type& vertex) const {
			typename graph_type::vertex_equal_type cmp;
			typename graph_type::vertex_list_type result;
			this->findEdges([&](const typename graph_type::edge_type& edge) {
				bool src = cmp(edge->getSource(), vertex);
				bool dst = cmp(edge->getTarget(), vertex);
				if (!src && !dst) return false;
				// in order to cope with vertices which build a cycle with themselfs!
				if (!src) result.push_back(edge->getSource());
				else if (!dst) result.push_back(edge->getTarget());
				return false;
			});
			return result;
		}

		bool operator ==(const Graph<TVertex, undirected>& other) const {
			return detail::equals_unordered<TVertex, undirected>(this->getVertices(), other.getVertices()) &&
						 detail::equals_unordered<TVertex, undirected>(this->getEdges(), other.getEdges());
		}

		bool operator !=(const Graph<TVertex, undirected>& other) const {
			return !(*this == other);
		}
	};
	template<typename TVertex>
	using UndirectedGraph = Graph<TVertex, undirected>;

	template<typename TVertex, typename TDirection>
	class GraphPrinter : public Printable {
		const Graph<TVertex, TDirection>& graph;
	public:
		typedef typename Graph<TVertex, TDirection>::vertex_type vertex_type;
		typedef typename Graph<TVertex, TDirection>::edge_type edge_type;
		GraphPrinter(const Graph<TVertex, TDirection>& graph) : graph(graph) {}
		std::ostream& printTo(std::ostream& stream) const override;
	protected:
		virtual std::string getGraphLabel() const = 0;
		virtual std::string getVertexId(const vertex_type& vertex) const = 0;
		virtual std::string getVertexLabel(const vertex_type& vertex) const = 0;
		virtual std::string getEdgeLabel(const edge_type& edge) const = 0;
		virtual std::string getVertexAttributes(const vertex_type& vertex) const { return ""; }
	private:
		std::string getEdgeId(const edge_type& edge) const;

		template<typename U = TDirection>
		typename std::enable_if<std::is_same<U, directed>::value, std::string>::type
		getGraphType() const { return "digraph "; }

		template<typename U = TDirection>
		typename std::enable_if<std::is_same<U, undirected>::value, std::string>::type
		getGraphType() const { return "graph "; }

		template<typename U = TDirection>
		typename std::enable_if<std::is_same<U, directed>::value, std::string>::type
		getEdgeConnector() const { return " -> "; }

		template<typename U = TDirection>
		typename std::enable_if<std::is_same<U, undirected>::value, std::string>::type
		getEdgeConnector() const { return " -- "; }
	};

	template<typename TVertex, typename TDirection>
	std::ostream& GraphPrinter<TVertex, TDirection>::printTo(std::ostream& stream) const {
		stream << getGraphType() << getGraphLabel() << " {" << std::endl;

		for (const auto& vertex: graph.getVertices())
			stream << getVertexId(vertex) << " [label=\"" << getVertexLabel(vertex)
				<< "\", shape=\"rectangle\"" << getVertexAttributes(vertex) << "]" << std::endl;

		for (const auto& edge: graph.getEdges())
			stream << getEdgeId(edge) << " [label=\"" << getEdgeLabel(edge) << "\"]" << std::endl;

		stream << "}" << std::endl;
		return stream;
	}

	template<typename TVertex, typename TDirection>
	std::string GraphPrinter<TVertex, TDirection>::getEdgeId(const edge_type& edge) const {
		return getVertexId(edge->getSource()) + getEdgeConnector() + getVertexId(edge->getTarget());
	}
}
