#pragma once
#include "core/core.h"

namespace core {
namespace analysis {
namespace callgraph {
  template<typename Lambda>
  typename std::enable_if<
    std::is_same<typename std::result_of<Lambda(const FunctionPtr&)>::type, bool>::value,
    optional<FunctionPtr>
  >::type findFunction(const ProgramPtr& program, Lambda lambda) {
    const auto& funs = program->getFunctions();
    auto it = std::find_if(std::begin(funs), std::end(funs),
      [&](const FunctionPtr& fun) -> bool { return lambda(fun); });
    if (it == std::end(funs)) return {};
    else                      return *it;
  }

  optional<FunctionPtr> findFunction(const ProgramPtr& program, const std::string& name);
  FunctionPtr getMainFunction(const ProgramPtr& program);
  bool isMainFunction(const FunctionPtr& fun);
  bool isExternalFunction(const FunctionPtr& fun);
  bool isAnonymousFunction(const FunctionPtr& fun);
  std::string getFunctionName(const FunctionPtr& fun);

  typedef DirectedGraph<Function> CallGraph;
  CallGraph getCallGraph(const ProgramPtr& program);

  class CallGraphPrinter : public GraphPrinter<Function, directed> {
	public:
		CallGraphPrinter(const CallGraph& callGraph) :
			GraphPrinter(callGraph)
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
	std::string toString(const core::analysis::callgraph::CallGraph& cg);
}
