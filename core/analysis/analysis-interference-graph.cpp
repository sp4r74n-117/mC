#include "core/analysis/analysis-interference-graph.h"
#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-live-variable.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis.h"

namespace core {
namespace analysis {
namespace interference {
  InterferenceGraph getInterferenceGraph(const core::FunctionPtr& fun, Type::TypeId type,
    const worklist::InsnLiveness& liveness, const InsnList& insns) {
    using namespace core::analysis::worklist;
    InterferenceGraph result;

    auto pred = [&](const VariablePtr& var) {
      // names of static arrays at not allowed to be mapped!
      return types::isType(var->getType(), type) ||
            (types::isArray(var->getType()) && !var->getParent()->isConst() && type == Type::TI_Int);
    };
    // live ranges of parameters always interfere! sadly N^2 ..
    auto params = fun->getParameters();
    for (const auto& p1 : params) {
      if (!pred(p1)) continue;
      for (const auto& p2 : params) {
        if (*p1 == *p2) continue;
        result.addEdge(p1, p2);
      }
    }

    const auto& nodeData = liveness.getNodeData();
    for (const auto& insn : insns) {
      // construct an undirected graph consisting of:
      // 1. A node for each variable
      // 2. An edge between v1 and v2 if they are live
      // simultaneously at some point in the program
      auto output = insn::getOutputVars(insn, pred);
      if (output.empty()) continue;
      // an instruction may only have one output
      assert(output.size() == 1 && "insns may only have at most one output");

      auto lhs = *output.begin();
      // check if the type is of interest
      if (!types::isType(lhs->getType(), type)) continue;
      // add the vertex regardless of liveOut set
      result.addVertex(lhs);
      // construct all required edges
      auto it = nodeData.find(insn);
      if (it != nodeData.end()) {
        for (const auto& var : it->second->getLiveOut()) {
          if (*var == *lhs || !pred(var)) continue;
          // check types as well as we do not build an edge to a different type
          if (!types::isType(var->getType(), type)) continue;
          result.addEdge(lhs, var);
        }
      }
    }
    return result;
  }

  std::string InterferenceGraphPrinter::getVertexId(const vertex_type& vertex) const {
    std::string result = toString(*vertex);
    if (core::analysis::isTemporary(vertex)) result.replace(0, 1, "t");
    else std::replace(result.begin(), result.end(), '.', '_');
    return result;
  }
}
}
}
