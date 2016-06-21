#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-insn.h"

namespace core {
namespace analysis {
namespace callgraph {
  optional<FunctionPtr> findFunction(const ProgramPtr& program, const std::string& name) {
    return findFunction(program, [&](const FunctionPtr& fun) { return fun->getLabel()->getName() == name; });
  }

  bool isMainFunction(const FunctionPtr& fun) {
    return fun->getName() == "_main";
  }

  bool isExternalFunction(const FunctionPtr& fun) {
    return fun->getBasicBlocks().size() == 0;
  }

  bool isAnonymousFunction(const FunctionPtr& fun) {
    return fun->getName() == "";
  }

  std::string getFunctionName(const FunctionPtr& fun) {
    if (isAnonymousFunction(fun)) return "_";
    else                          return fun->getName();
  }

  FunctionPtr getMainFunction(const ProgramPtr& program) {
    assert(!program->getFunctions().empty() && "invalid instance of NodeManager passed");

    const auto& funs = program->getFunctions();
    if (funs.size() == 1 && isAnonymousFunction(funs.front()))
      return funs.front();

    // find the function named main, otherwise there is an error!
    auto fun = findFunction(program,
      [&](const FunctionPtr& fun) { return isMainFunction(fun); });
    assert(fun && "failed to find main function");
    return *fun;
  }

  CallGraph getCallGraph(const ProgramPtr& program) {
    CallGraph result;
    for (const auto& fun : program->getFunctions()) {
      // insert the function as well as it might be that a function exists
      // but is never called
      result.addVertex(fun);
      // process all basic blocks within the given function
      for (const auto& bb : fun->getBasicBlocks()) {
        // process all insns and create edges
        for (const auto& insn : bb->getInsns()) {
          auto callee = insn::getCallTarget(insn);
          if (!callee) continue;

          result.addEdge(fun, *callee);
        }
      }
    }
    return result;
  }

  std::string CallGraphPrinter::getGraphLabel() const {
    return "callgraph";
  }

  std::string CallGraphPrinter::getVertexId(const vertex_type& vertex) const {
    return getFunctionName(vertex);
  }

  std::string CallGraphPrinter::getVertexLabel(const vertex_type& vertex) const {
    return getVertexId(vertex);
  }

  std::string CallGraphPrinter::getEdgeLabel(const edge_type& edge) const {
    return "";
  }
}
}
}
namespace utils {
	std::string toString(const core::analysis::callgraph::CallGraph& cg) {
    std::stringstream ss;
    core::analysis::callgraph::CallGraphPrinter(cg).printTo(ss);
    return ss.str();
  }
}
