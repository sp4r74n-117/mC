#pragma once
#include "core/core.h"
#include "core/analysis/analysis-live-variable.h"
#include "utils/utils-graph-color.h"

namespace core {
namespace analysis {
namespace interference {
  typedef graph::color::ColorGraph<Variable> InterferenceGraph;
  InterferenceGraph getInterferenceGraph(const core::FunctionPtr& fun, Type::TypeId type,
    const worklist::InsnLiveness& liveness, const InsnList& insns);

  class InterferenceGraphPrinter : public graph::color::ColorGraphPrinter<Variable> {
  public:
    InterferenceGraphPrinter(const graph::color::ColorGraph<Variable>& graph,
      const graph::color::Mappings<Variable>& mappings) :
      ColorGraphPrinter(graph, mappings) {}
  protected:
    std::string getVertexId(const vertex_type& vertex) const override;
  };
}
}
}
