#pragma once
#include "core/core.h"

namespace backend {
namespace insn {
  class TemplateInsn;
  typedef Ptr<TemplateInsn> TemplateInsnPtr;
}
  class Backend;
  typedef Ptr<Backend> BackendPtr;
  class PatternMatcher;
  typedef Ptr<PatternMatcher> PatternMatcherPtr;
  class PatternContext;
  typedef Ptr<PatternContext> PatternContextPtr;

  class Backend : public Printable {
    core::ProgramPtr program;
    bool instrument;
    bool regalloc;
  public:
    virtual bool convert() = 0;
    const core::ProgramPtr& getProgram() const { return program; }
    void setInstrument(bool enable) { instrument = enable; }
    bool getInstrument() const { return instrument; }
    void setRegAlloc(bool enable) { regalloc = enable; }
    bool getRegAlloc() const { return regalloc; }
  protected:
    Backend(const core::ProgramPtr& program) :
      program(program), instrument(false), regalloc(true)
    { }
  };

  class PatternContext {};

  class PatternResult : public std::pair<insn::TemplateInsnPtr, unsigned> {
  public:
    using std::pair<insn::TemplateInsnPtr, unsigned>::pair;
    const insn::TemplateInsnPtr& getInsn() const { return first; }
    unsigned getCount() const { return second; }
  };

  PatternResult makeResult(const insn::TemplateInsnPtr& insn, unsigned count = 1);

  class PatternMatcher {
  public:
    virtual ~PatternMatcher() {}
    virtual bool matches(const core::InsnPtr& insn) const = 0;
    virtual PatternResult generate(const core::InsnPtr& insn) const = 0;
  protected:
    PatternMatcher(const PatternContextPtr& context) : context(context) {}
    PatternContextPtr context;
  };

  bool dumpTo(const BackendPtr& backend, const std::string& dir);
  BackendPtr makeSimpleBackend(const core::ProgramPtr& program);
  BackendPtr makeRegAllocBackend(const core::ProgramPtr& program);
  BackendPtr makeDefaultBackend(const core::ProgramPtr& program);
}
