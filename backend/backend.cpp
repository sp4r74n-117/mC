#include "backend/backend.h"
#include "backend/backend-simple.h"
#include "backend/backend-regalloc.h"
#include "core/analysis/analysis-interference-graph.h"
#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-live-variable.h"
#include <fstream>
#include <cstdlib>

namespace backend {
  PatternResult makeResult(const insn::TemplateInsnPtr& insn, unsigned count) {
    return {insn, count};
  }

  BackendPtr makeSimpleBackend(const core::ProgramPtr& program) {
    return std::make_shared<simple::SimpleBackend>(program);
  }

  BackendPtr makeRegAllocBackend(const core::ProgramPtr& program) {
    return std::make_shared<regalloc::RegAllocBackend>(program);
  }

  BackendPtr makeDefaultBackend(const core::ProgramPtr& program) {
    return makeRegAllocBackend(program);
  }

  namespace {
    bool dumpTo(const std::string& file, const std::string& text) {
        std::ofstream of{file};
        if (!of) return false;
        of << text;
        if (!of) return false;
        // and also generate a png of it automatically
        of.close();
        return dot::generatePng(file);
    }

    bool dumpTo(const std::string& file, const core::FunctionPtr& fun) {
      auto insns = core::analysis::controlflow::getLinearInsnList(fun);
      // use insns to generate the liveness information
      core::analysis::worklist::InsnLiveness liveness;
      liveness.apply(insns.begin(), insns.end());
      auto graph = core::analysis::interference::getInterferenceGraph(fun, core::Type::TI_Int, liveness, insns);
      // use 4 colors as the backend-regalloc does so as well
      auto color = graph::color::getColorMappings(graph, 4);

      std::stringstream ss;
      core::analysis::interference::InterferenceGraphPrinter(graph, color).printTo(ss);
      return dumpTo(file, ss.str());
    }
  }

  bool dumpTo(const BackendPtr& backend, const std::string& dir) {
    std::string name = "/program.s";
    std::ofstream of{dir + name};
    if (!of) return false;
    backend->printTo(of);
    if (!of) return false;
    // and also generate a png of it automatically
    of.close();
    // finally, generate a color mapping file for each function
		for (const auto& fun : backend->getProgram()->getFunctions()) {
			if (!dumpTo(dir + "/" + core::analysis::callgraph::getFunctionName(fun) + ".dot", fun)) return false;
		}
    return false;
  }
}
