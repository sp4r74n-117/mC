#include "backend/backend-regalloc.h"
#include "backend/backend-memory.h"
#include "backend/backend-insn.h"
#include "backend/backend-instrument.h"
#include "core/analysis/analysis.h"
#include "core/analysis/analysis-insn.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-controlflow.h"
#include "core/analysis/analysis-interference-graph.h"
#include "core/analysis/analysis-live-variable.h"
#include "core/arithmetic/arithmetic.h"

namespace backend {
namespace regalloc {
  namespace detail {
    // Callee-Saved vs Caller-Saved in __cdecl
    // Registers EAX, ECX, and EDX are caller-saved, and the rest are callee-saved
    // o caller-saved registers are used to hold temporary quantities
    //   that need not be preserved across calls.
    // o callee-saved registers are used to hold long-lived values
    //   that should be preserved across calls.
    bool isCallerSaved(insn::MachineOperand::Register reg) {
      switch (reg) {
      case insn::MachineOperand::OR_Eax:
      case insn::MachineOperand::OR_Ecx:
      case insn::MachineOperand::OR_Edx:
          return true;
      default:
          return false;
      }
    }

    bool isCalleeSaved(insn::MachineOperand::Register reg) {
      return !isCallerSaved(reg);
    }

    insn::MachineOperand::Register mapColor(int color) {
      switch (color) {
      case 0: return insn::MachineOperand::OR_Ebx;
      case 1: return insn::MachineOperand::OR_Edi;
      case 2: return insn::MachineOperand::OR_Esi;
      case 3: return insn::MachineOperand::OR_Edx;
      }
      assert(false && "invalid color to reg mapping");
      return insn::MachineOperand::OR_Eax;
    }

    template<typename Lambda, typename TResult = std::set<insn::MachineOperand::Register>>
    TResult mapColors(const graph::color::Mappings<core::Variable>& mappings, Lambda lambda) {
      TResult result;
      // determine, user defined, which registers we need to map
      for (const auto& mapping : mappings) {
        if (mapping.color < 0) continue;

        auto reg = mapColor(mapping.color);
        if (lambda(reg)) result.insert(reg);
      }
      return result;
    }
  }

  class RegAllocContext : public PatternContext {
    RegAllocBackend& backend;
    memory::StackFramePtr frame;
    graph::color::Mappings<core::Variable> intMapping;
    core::analysis::worklist::InsnLiveness liveness;
  public:
    RegAllocContext(RegAllocBackend& backend) : backend(backend) {}
    const RegAllocBackend& getBackend() const { return backend; }
    const memory::StackFramePtr& getFrame() const { return frame; }
    void setFrame(const memory::StackFramePtr& frame) { this->frame = frame; }
    const graph::color::Mappings<core::Variable>& getIntMapping() const { return intMapping; }
    void setIntMapping(const graph::color::Mappings<core::Variable>& mapping) { intMapping = mapping; }
    const core::analysis::worklist::InsnLiveness& getLiveness() const { return liveness; }
    void setLiveness(const core::analysis::worklist::InsnLiveness& liveness) { this->liveness = liveness; }
  };

  class RegAllocMatcher : public PatternMatcher {
  public:
    RegAllocMatcher(const PatternContextPtr& context) : PatternMatcher(context) {}
    RegAllocContextPtr getContext() const { return cast<RegAllocContext>(context); }

    insn::TemplateInsnPtr buildFrameEntryTemplate() const {
      const auto& frame = getContext()->getFrame();
      const auto& intMapping = getContext()->getIntMapping();

      auto result = insn::buildFrameEntryTemplate(frame);
      // extend it to preserve the additional registers
      for (const auto& reg : detail::mapColors(intMapping, &detail::isCalleeSaved)) {
        insn::TemplateInsn::append(result, insn::buildPushTemplate(
          insn::buildRegOperand(reg, insn::MachineOperand::OS_32Bit)));
      }
      // add instrumentation
      if (getContext()->getBackend().getInstrument())
        insn::TemplateInsn::append(result, instrument::buildInstrumentationEntryTemplate());
      // fetch all parameters which are mapped to registers
      const auto& params = frame->getParameters();
      for (const auto& mapping : intMapping) {
        for (const auto& param : params) {
          if (*mapping.vertex != *param || mapping.color < 0) continue;
          // we found it, so fetch now
          insn::TemplateInsn::append(result, insn::buildMovTemplate(
            insn::buildMemOperand(frame->getRelativeOffset(param)),
            insn::buildRegOperand(detail::mapColor(mapping.color), insn::MachineOperand::OS_32Bit)));
        }
      }
      return result;
    }

    insn::TemplateInsnPtr buildFrameLeaveTemplate(bool preserveEax) const {
      const auto& frame = getContext()->getFrame();
      const auto& intMapping = getContext()->getIntMapping();

      auto result = insn::buildFrameLeaveTemplate(frame);
      // extend it to preserve the additional registers
      for (const auto& reg : detail::mapColors(intMapping, &detail::isCalleeSaved)) {
        insn::TemplateInsn::prepend(result, insn::buildPopTemplate(
          insn::buildRegOperand(reg, insn::MachineOperand::OS_32Bit)));
      }
	    // add instrumentation
      if (getContext()->getBackend().getInstrument()) {
        auto eax = insn::buildRegOperand(insn::MachineOperand::OR_Eax, insn::MachineOperand::OS_32Bit);
        // in case we are not allowed to clobber %eax, preserve it!
        if (preserveEax) insn::TemplateInsn::prepend(result, insn::buildPopTemplate(eax));
        insn::TemplateInsn::prepend(result, instrument::buildInstrumentationLeaveTemplate());
        if (preserveEax) insn::TemplateInsn::prepend(result, insn::buildPushTemplate(eax));
      }
      return result;
    }

    struct MapIngredients {
      MapIngredients() :
        intReg(insn::MachineOperand::OR_Eax), fltReg(insn::MachineOperand::OR_Xmm0),
        allowImm(false), allowMem(false), allowReg(false) {}

      insn::MachineOperand::Register intReg; // used int reg iff required
      insn::MachineOperand::Register fltReg; // used flt reg iff required
      bool allowImm; // allows to return an ImmOperand
      bool allowMem; // allows to return a MemOperand
      bool allowReg; // allows to return the color mapped RegOperand
    };

    insn::MachineOperandPtr mapOperand(insn::MachineInsnList& insns, const core::ValuePtr& value, const MapIngredients& ingredients) const {
      const auto& frame = getContext()->getFrame();
      const auto& intMapping = getContext()->getIntMapping();

      if (core::analysis::isConstant(value)) {
        // ints which are read only can be mapped to imms straight away
        if (ingredients.allowImm && core::analysis::types::isInt(value->getType())) return insn::buildImmOperand(value);
        // use the provided regs to map them into a working register
        auto tmp0 = insn::buildRegOperand(core::analysis::types::isInt(value->getType()) ?
          ingredients.intReg : ingredients.fltReg);
        appendAll(insns, insn::buildMovTemplate(insn::buildImmOperand(value), tmp0)->getInsns());
        return tmp0;
      }

      auto var = cast<core::Variable>(value);
      insn::MachineOperand::Register reg = ingredients.intReg;
      if (core::analysis::types::isFloat(core::analysis::types::getElementType(var->getType())) &&
          !core::analysis::types::isArray(var->getType()) && !core::analysis::isOffset(value))
          reg = ingredients.fltReg;
      auto dst = insn::buildRegOperand(reg);
      if (reg == ingredients.intReg) {
        // lookup in register mapping to find an assigned register
        auto it = std::find_if(intMapping.begin(), intMapping.end(),
          [&](const auto& mapping) { return *mapping.vertex == *var && mapping.color >= 0; });
        if (it != intMapping.end()) {
          // we have a mapping, thus take use of it and return to the user
          auto src = insn::buildRegOperand(detail::mapColor(it->color), insn::MachineOperand::OS_32Bit);
          // in case we assure only to read from, return it right away
          if (ingredients.allowReg) return src;
          // generate via std move
          appendAll(insns, insn::buildMovTemplate(src, dst)->getInsns());
          return dst;
        }
        // default fall-through
      }

      auto src = insn::buildMemOperand(frame->getRelativeOffset(var));
      if (ingredients.allowMem) return src;
      // otherwise move into the designated register
      appendAll(insns, insn::buildMovTemplate(src, dst)->getInsns());
      return dst;
    }

    insn::MachineOperandPtr mapRValue(insn::MachineInsnList& insns, const core::ValuePtr& value,
      insn::MachineOperand::Register intReg, insn::MachineOperand::Register fltReg, bool readOnly = false, bool allowMem = false) const {
      MapIngredients ingredients;
      ingredients.intReg = intReg;
      ingredients.fltReg = fltReg;
      ingredients.allowImm = readOnly;
      ingredients.allowReg = readOnly;
      ingredients.allowMem = allowMem;
      return mapOperand(insns, value, ingredients);
    }

    insn::MachineOperandPtr mapLValue(const core::VariablePtr& var) const {
      MapIngredients ingredients;
      ingredients.allowMem = true;
      ingredients.allowReg = true;

      insn::MachineInsnList insns;
      return mapOperand(insns, var, ingredients);
    }
  };

  template<typename TMatcher, typename... TArgs>
  RegAllocMatcherPtr makeMatcher(TArgs&&... args) {
    return std::make_shared<TMatcher>(std::forward<TArgs>(args)...);
  }

  class PlainAssignMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      if (!core::analysis::insn::isAssignInsn(insn)) return false;

      auto assign = cast<core::AssignInsn>(insn);
      return assign->isAssign();
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto assign = cast<core::AssignInsn>(insn);
      // input can be the following form
      // {v0,$0} = {imm,$0,v0}
      insn::MachineInsnList insns;
      // liveness?
      bool omit = false;
      const auto& liveness = getContext()->getLiveness();
      if (!liveness.getNodeData().empty()) {
        const auto& liveOut = liveness.getNodeData().at(insn)->getLiveOut();
        if (liveOut.find(assign->getLhs()) == liveOut.end()) omit = true;
      }

      if (!omit) {
        // generate the target memory operand, this can be done independent of cases
        auto dst = mapLValue(assign->getLhs());
        // generate a register "swap" due to the semantics of mapRValue(readOnly = false)
        // 80485a8:	e8 0e ff ff ff       	call   80484bb <read_int>
        // 80485ad:	89 c3                	mov    %eax,%ebx
        // 80485af:	89 d8                	mov    %ebx,%eax
        // 80485b1:	89 45 f4             	mov    %eax,-0xc(%ebp)
        auto src = mapRValue(insns, assign->getRhs1(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0, true);
        // now move the operand into the destination
        appendAll(insns, insn::buildMovTemplate(src, dst)->getInsns());
      }
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class UnaryAssignMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      if (!core::analysis::insn::isAssignInsn(insn)) return false;

      auto assign = cast<core::AssignInsn>(insn);
      return assign->isUnary();
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto assign = cast<core::AssignInsn>(insn);

      insn::MachineInsnList insns;
      auto lhs = mapLValue(assign->getLhs());
      // generic right hand side, depending on matching we have two cases
      insn::MachineOperandPtr rhs;
      // in case we face a constant we can generate better code
      if (core::analysis::isConstant(assign->getRhs1())) {
        switch (assign->getOp()) {
        case core::AssignInsn::SUB:
          {
            // negate regardless of type
            auto value = (-1.0f) * core::arithmetic::getValue<float>(assign->getRhs1());
            if (core::analysis::isIntConstant(assign->getRhs1()))
              rhs = insn::buildImmOperand(static_cast<int>(value));
            else
              rhs = insn::buildImmOperand(value);
            break;
          }
        case core::AssignInsn::NOT:
            // it must be an int, otherwise it makes no sense
            assert(core::analysis::isIntConstant(assign->getRhs1()) &&
              "logical not requires an integer value");
            rhs = insn::buildImmOperand((-1) * core::arithmetic::getValue<int>(assign->getRhs1()));
            break;
        default:
            assert(false && "unsupported unary operation");
            return makeResult({});
        }
      } else {
        rhs = mapRValue(insns, assign->getRhs1(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0);
        switch (assign->getOp()) {
        case core::AssignInsn::SUB:
            appendAll(insns, insn::buildNegTemplate(rhs)->getInsns());
            break;
        case core::AssignInsn::NOT:
            appendAll(insns, insn::buildNotTemplate(rhs)->getInsns());
            break;
        default:
            assert(false && "unsupported unary operation");
            return makeResult({});
        }
      }
      appendAll(insns, insn::buildMovTemplate(rhs, lhs)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class BinaryArithmeticShiftMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      if (!core::analysis::insn::isAssignInsn(insn)) return false;

      auto assign = cast<core::AssignInsn>(insn);
      if (!assign->isBinary()) return false;
      // mul or dir
      if (assign->getOp() != core::AssignInsn::DIV && assign->getOp() != core::AssignInsn::MUL) return false;
      // assumes passes-normalize has been run already
      if (!core::analysis::isIntConstant(assign->getRhs2())) return false;

      auto value = core::arithmetic::getValue<int>(assign->getRhs2());
      if (!(value <= 0xFF && value % 2 == 0)) return false;
      // assure that is is right
      return value == std::pow(2, static_cast<int>(std::log2(value)));
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto assign = cast<core::AssignInsn>(insn);
      // expected input:
      // $0 = rhs1 * rhs2
      insn::MachineInsnList insns;
      auto lhs  = mapLValue(assign->getLhs());
      auto rhs1 = mapRValue(insns, assign->getRhs1(),
        lhs->isRegister() ? lhs->getRegister() : insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0);
      assert(rhs1->isInt() && "arithmetic shift only possible for integers");

      auto value = core::arithmetic::getValue<int>(assign->getRhs2());
      // compute the shift and build the operand
      auto shift = insn::buildImmOperand(static_cast<int>(std::log2(value)));
      switch (assign->getOp()) {
      case core::AssignInsn::MUL:
          insns.push_back(insn::buildSalInsn(shift, rhs1));
          break;
      case core::AssignInsn::DIV:
          insns.push_back(insn::buildSarInsn(shift, rhs1));
          break;
      default:
          assert(false && "unsupported op for shift reduction");
          break;
      }
      // result is in %eax this move to dest in case we did not optimize
      if (*lhs != *rhs1) appendAll(insns, insn::buildMovTemplate(rhs1, lhs)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class BinaryArithmeticAddSubMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      if (!core::analysis::insn::isAssignInsn(insn)) return false;

      auto assign = cast<core::AssignInsn>(insn);
      if (!assign->isBinary()) return false;
      if (core::analysis::isOffset(assign->getLhs())) return false;
      return (assign->getOp() == core::AssignInsn::ADD) ||
             (assign->getOp() == core::AssignInsn::SUB);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto assign = cast<core::AssignInsn>(insn);
      // expected input:
      // {v0,$0} = rhs1 op rhs2
      insn::MachineInsnList insns;
      auto lhs  = mapLValue(assign->getLhs());
      // according to the mnemonic we can also use mem operands
      auto rhs2 = mapRValue(insns, assign->getRhs2(), insn::MachineOperand::OR_Ecx, insn::MachineOperand::OR_Xmm1, true, true);

      insn::MachineOperandPtr rhs1;
      if (*assign->getLhs() == *assign->getRhs1() && core::analysis::isIntConstant(assign->getRhs2())) {
        // rhs1 can be flattened into lhs itself
        rhs1 = lhs;
      } else {
        rhs1 = mapRValue(insns, assign->getRhs1(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0,
          // in case source and dest maps to the same we can flatten
          *assign->getLhs() == *assign->getRhs1());
      }

      switch (assign->getOp()) {
      case core::AssignInsn::ADD: appendAll(insns, insn::buildAddTemplate(rhs2, rhs1)->getInsns()); break;
      case core::AssignInsn::SUB: appendAll(insns, insn::buildSubTemplate(rhs2, rhs1)->getInsns()); break;
      default:
          assert(false && "unsupported binary operation");
          break;
      }
      // result is in %eax or %xmm0 this move to dest in case we did not optimize
      if (*lhs != *rhs1) appendAll(insns, insn::buildMovTemplate(rhs1, lhs)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class BinaryArithmeticMulDivMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      if (!core::analysis::insn::isAssignInsn(insn)) return false;

      auto assign = cast<core::AssignInsn>(insn);
      if (!assign->isBinary()) return false;
      return (assign->getOp() == core::AssignInsn::MUL) ||
             (assign->getOp() == core::AssignInsn::DIV);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      const auto& intMapping = getContext()->getIntMapping();
      auto assign = cast<core::AssignInsn>(insn);
      // expected input:
      // {v0,$0} = rhs1 op rhs2
      insn::MachineInsnList insns;
      auto lhs  = mapLValue(assign->getLhs());
      auto rhs1 = mapRValue(insns, assign->getRhs1(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0,
        // in case source and dest maps to the same we can flatten
        *assign->getLhs() == *assign->getRhs1());
      auto rhs2 = mapRValue(insns, assign->getRhs2(), insn::MachineOperand::OR_Ecx, insn::MachineOperand::OR_Xmm1,
        // only exception is idiv as the mnemonic is e.g. r/m32
        !(core::analysis::types::isInt(assign->getRhs2()->getType()) && (assign->getOp() == core::AssignInsn::DIV)),
        // in case of float a mem location is just fine!
        core::analysis::types::isFloat(assign->getRhs2()->getType()));
      switch (assign->getOp()) {
      case core::AssignInsn::MUL: appendAll(insns, insn::buildMulTemplate(rhs2, rhs1)->getInsns()); break;
      case core::AssignInsn::DIV:
        {
          bool saveEdx = false;
          auto edx = insn::buildRegOperand(insn::MachineOperand::OR_Edx, insn::MachineOperand::OS_32Bit);
          // check if we need to save EDX as idiv clobbers it
          if (rhs2->isInt()) {
            auto regs = detail::mapColors(intMapping, &detail::isCallerSaved);
            saveEdx = regs.find(insn::MachineOperand::OR_Edx) != regs.end();
          }
          if (saveEdx) appendAll(insns, insn::buildPushTemplate(edx)->getInsns());
          appendAll(insns, insn::buildDivTemplate(rhs2, rhs1)->getInsns());
          if (saveEdx) appendAll(insns, insn::buildPopTemplate(edx)->getInsns());
          break;
        }
      default:
          assert(false && "unsupported binary operation");
          break;
      }
      // result is in %eax or %xmm0 this move to dest in case we did not optimize
      if (*lhs != *rhs1) appendAll(insns, insn::buildMovTemplate(rhs1, lhs)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class BinaryLogicalAssignMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      if (!core::analysis::insn::isAssignInsn(insn)) return false;

      auto assign = cast<core::AssignInsn>(insn);
      return assign->isBinary() && core::AssignInsn::isLogicalBinaryOp(assign->getOp());
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto assign = cast<core::AssignInsn>(insn);
      // expected input:
      // {v0,$0} = rhs1 op rhs2
      insn::MachineInsnList insns;
      auto lhs  = mapLValue(assign->getLhs());
      auto rhs1 = mapRValue(insns, assign->getRhs1(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0,
        !core::analysis::isConstant(assign->getRhs1()));
      auto rhs2 = mapRValue(insns, assign->getRhs2(), insn::MachineOperand::OR_Ecx, insn::MachineOperand::OR_Xmm1, true, true);
      // required in case of logical ops
      auto al  = insn::buildRegOperand(insn::MachineOperand::OR_Eax, insn::MachineOperand::OS_8Bit);
      auto eax = insn::buildRegOperand(al, insn::MachineOperand::OS_32Bit);
      switch (assign->getOp()) {
      case core::AssignInsn::EQ: appendAll(insns, insn::buildEqualTemplate(rhs2, rhs1, al)->getInsns()); break;
      case core::AssignInsn::NE: appendAll(insns, insn::buildNotEqualTemplate(rhs2, rhs1, al)->getInsns()); break;
      case core::AssignInsn::LE: appendAll(insns, insn::buildLessEqualTemplate(rhs2, rhs1, al)->getInsns()); break;
      case core::AssignInsn::LT: appendAll(insns, insn::buildLessTemplate(rhs2, rhs1, al)->getInsns()); break;
      case core::AssignInsn::GE: appendAll(insns, insn::buildGreaterEqualTemplate(rhs2, rhs1, al)->getInsns()); break;
      case core::AssignInsn::GT: appendAll(insns, insn::buildGreaterTemplate(rhs2, rhs1, al)->getInsns()); break;
      default:
          assert(false && "unsupported binary operation");
          break;
      }
      // extend %al into lhs first
      appendAll(insns, insn::buildMovTemplate(al, eax)->getInsns());
      // and move to the target location afterwards
      appendAll(insns, insn::buildMovTemplate(eax, lhs)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class BinaryLogicalJumpMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      const auto& liveness = getContext()->getLiveness();
      if (liveness.getNodeData().empty()) return false;
      // check for an assignment, this must be the case anyway
      if (!core::analysis::insn::isAssignInsn(insn)) return false;
      // logical binary?
      auto assign = cast<core::AssignInsn>(insn);
      if (!(assign->isBinary() && core::AssignInsn::isLogicalBinaryOp(assign->getOp()))) return false;
      // check if the next one is a jump where the condition is not live afterwards
      auto next = core::analysis::insn::getSuccessors(insn);
      if (next.size() != 1) return false;
      // it must be a jump where the latter condition matches
      if (!core::analysis::insn::isFalseJumpInsn(*next.begin())) return false;
      auto fjmp = cast<core::FalseJumpInsn>(*next.begin());
      // do lhs & cond match?
      if (*fjmp->getCond() != *assign->getLhs()) return false;
      // liveness?
      const auto& liveOut = liveness.getNodeData().at(fjmp)->getLiveOut();
      return liveOut.find(cast<core::Variable>(fjmp->getCond())) == liveOut.end();
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto assign = cast<core::AssignInsn>(insn);
      // expected input:
      // $0 = rhs1 op rhs2
      // fjmp $0 Lx
      insn::MachineInsnList insns;
      auto lhs  = mapLValue(assign->getLhs());
      // optimize ..
      auto rhs1 = mapRValue(insns, assign->getRhs1(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0,
        !core::analysis::isConstant(assign->getRhs1()));
      auto rhs2 = mapRValue(insns, assign->getRhs2(), insn::MachineOperand::OR_Ecx, insn::MachineOperand::OR_Xmm1, true, true);
      // grab the jfmp -- proven by matches() that this access is valid!
      auto fjmp = cast<core::FalseJumpInsn>(*core::analysis::insn::getSuccessors(insn).begin());
      auto target = insn::buildLocOperand(fjmp->getTarget()->getName());
      switch (assign->getOp()) {
      case core::AssignInsn::EQ: appendAll(insns, insn::buildJmpNotEqualTemplate(rhs2, rhs1, target)->getInsns()); break;
      case core::AssignInsn::NE: appendAll(insns, insn::buildJmpEqualTemplate(rhs2, rhs1, target)->getInsns()); break;
      case core::AssignInsn::LE: appendAll(insns, insn::buildJmpGreaterTemplate(rhs2, rhs1, target)->getInsns()); break;
      case core::AssignInsn::LT: appendAll(insns, insn::buildJmpGreaterEqualTemplate(rhs2, rhs1, target)->getInsns()); break;
      case core::AssignInsn::GE: appendAll(insns, insn::buildJmpLessTemplate(rhs2, rhs1, target)->getInsns()); break;
      case core::AssignInsn::GT: appendAll(insns, insn::buildJmpLessEqualTemplate(rhs2, rhs1, target)->getInsns()); break;
      default:
          assert(false && "unsupported binary operation");
          break;
      }
      return makeResult(std::make_shared<insn::TemplateInsn>(insns), 2);
    }
  };

  class OffsetAssignMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      if (!core::analysis::insn::isAssignInsn(insn)) return false;

      auto assign = cast<core::AssignInsn>(insn);
      return core::analysis::isOffset(assign->getLhs());
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      const auto& frame = getContext()->getFrame();
      auto assign = cast<core::AssignInsn>(insn);
      // input can be the following form
      // $0 = v0 + $1
      insn::MachineInsnList insns;
      // fetch the address of v0 into eax
      auto rhs1 = assign->getRhs1();
      assert(core::analysis::types::isArray(rhs1->getType()) &&
        "offset assign may only be used with arrays!");
      insn::MachineOperandPtr base;
      // regardless of type, obtain the left side
      auto lhs = mapLValue(assign->getLhs());
      if (cast<core::Variable>(rhs1)->getParent()->isConst()) {
        // use lea to obtain the base address
        base = insn::buildRegOperand(insn::MachineOperand::OR_Eax);
        insns.push_back(insn::buildLeaInsn(insn::buildMemOperand(frame->getRelativeOffset(cast<core::Variable>(rhs1))), base));
      } else {
        base = mapRValue(insns, rhs1, insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0, true);
      }
      assert(base->isInt() && "base address must be stored in an int!");
      auto off = mapRValue(insns, assign->getRhs2(), insn::MachineOperand::OR_Ecx, insn::MachineOperand::OR_Xmm1);
      assert(off->isInt() && "offset must be stored in an int!");
      // compute the total offset which is still relative to ebp
      // prev ebp
      // a[n]     <- base + off (assuming off is >= 0)
      // ...
      // a[0]     <- base
      insns.push_back(insn::buildAddInsn(base, off));
      insns.push_back(insn::buildMovInsn(off, lhs));
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class ReturnMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isReturnInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto ret = cast<core::ReturnInsn>(insn);
      // input can be the following form
      // return {v0,$0,imm}
      insn::MachineInsnList insns;
      auto hasReturn = core::analysis::insn::hasReturnValue(ret);
      // generate the leave frame, regardless of different cases we will need it
      auto leave = buildFrameLeaveTemplate(hasReturn && core::analysis::types::isInt(ret->getRhs()->getType()));
      // check for fast-path return
      if (!hasReturn) {
        appendAll(insns, leave->getInsns());
        insns.push_back(insn::buildRetInsn());
        return makeResult(std::make_shared<insn::TemplateInsn>(insns));
      }

      // depending on type this is either %eax or %xmm0 -- do not specify readOnly=true as it would may change ABI!
      mapRValue(insns, ret->getRhs(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0);
      // finally add the leave frame and the actual ret instruction
      appendAll(insns, leave->getInsns());
      insns.push_back(insn::buildRetInsn());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class PushMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isPushInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto push = cast<core::PushInsn>(insn);
      return makeResult(insn::buildPushTemplate(core::analysis::isConstant(push->getRhs()) ?
        insn::buildImmOperand(push->getRhs()) :
        mapLValue(cast<core::Variable>(push->getRhs()))));
    }
  };

  class PopMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isPopInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto pop = cast<core::PopInsn>(insn);
      // check what type we need to generate
      // pop {imm,$0,v0}
      return makeResult(insn::buildPopTemplate(core::analysis::isConstant(pop->getRhs()) ?
        insn::buildImmOperand(core::arithmetic::getValue<int>(pop->getRhs())) :
        mapLValue(cast<core::Variable>(pop->getRhs()))));
    }
  };

  class PushSpMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isPushSpInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto push = cast<core::PushSpInsn>(insn);

      auto src = insn::buildRegOperand(insn::MachineOperand::OR_Esp);
      auto dst = mapLValue(push->getRhs());
      return makeResult(insn::buildMovTemplate(src, dst));
    }
  };

  class PopSpMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isPopSpInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto pop = cast<core::PopSpInsn>(insn);

      insn::MachineInsnList insns;
      auto src = mapRValue(insns, pop->getRhs(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0, true);
      auto dst = insn::buildRegOperand(insn::MachineOperand::OR_Esp);
      appendAll(insns, insn::buildMovTemplate(src, dst)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class CallMatcher : public RegAllocMatcher {
    void saveRegs(insn::MachineInsnList& insns, const std::set<insn::MachineOperand::Register>& regs) const {
      const auto& frame = getContext()->getFrame();
      for (const auto& reg: regs)
        // move the register to scratch space
        appendAll(insns, insn::buildMovTemplate(insn::buildRegOperand(reg, insn::MachineOperand::OS_32Bit),
          insn::buildMemOperand(frame->getRelativeOffset(reg)))->getInsns());
    }

    void restoreRegs(insn::MachineInsnList& insns, const std::set<insn::MachineOperand::Register>& regs) const {
      const auto& frame = getContext()->getFrame();
      // restore any caller saved regs
      for (const auto& reg: regs)
        // move the value from scratch space into the register again
        appendAll(insns, insn::buildMovTemplate(insn::buildMemOperand(frame->getRelativeOffset(reg)),
          insn::buildRegOperand(reg, insn::MachineOperand::OS_32Bit))->getInsns());
    }
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isCallInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      const auto& intMapping = getContext()->getIntMapping();
      auto call = cast<core::CallInsn>(insn);
      auto regs = detail::mapColors(intMapping, &detail::isCallerSaved);
      if (!core::analysis::insn::hasReturnValue(call)) {
        // simply generate the call and we are done
        insn::MachineInsnList insns;
        saveRegs(insns, regs);
        insns.push_back(insn::buildCallInsn(insn::buildLocOperand(mangle::demangle(call->getCallee()->getName()))));
        restoreRegs(insns, regs);
        return makeResult(std::make_shared<insn::TemplateInsn>(insns));
      }

      // in this case we get a result, either in %eax or %xmm0
      auto src = insn::buildRegOperand(core::analysis::types::isInt(call->getResult()->getType()) ?
        insn::MachineOperand::Register::OR_Eax : insn::MachineOperand::Register::OR_Xmm0,
        insn::MachineOperand::OS_32Bit);
      auto dst = mapLValue(call->getResult());

      insn::MachineInsnList insns;
      saveRegs(insns, regs);
      // issue the generic call
      insns.push_back(insn::buildCallInsn(insn::buildLocOperand(mangle::demangle(call->getCallee()->getName()))));
      appendAll(insns, insn::buildMovTemplate(src, dst)->getInsns());
      restoreRegs(insns, regs);
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class FalseJumpMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isFalseJumpInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto fjmp = cast<core::FalseJumpInsn>(insn);
      // expected input
      // fjmp {$0,v0,imm} label
      insn::MachineInsnList insns;
      auto rhs = mapRValue(insns, fjmp->getCond(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0, true);
      assert(rhs->isInt() && "fjmp condition must be provided in a 32-bit gpr");

      appendAll(insns, insn::buildCmpTemplate(insn::buildImmOperand(0), rhs)->getInsns());
      insns.push_back(insn::buildJmpEqualInsn(insn::buildLocOperand(fjmp->getTarget()->getName())));
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class GotoMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isGotoInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto ujmp = cast<core::GotoInsn>(insn);
      // expected input
      // goto label
      insn::MachineInsnList insns;
      insns.push_back(insn::buildJmpInsn(insn::buildLocOperand(ujmp->getTarget()->getName())));
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class LoadMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isLoadInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto load = cast<core::LoadInsn>(insn);
      // expected
      // load {$0,v0},$1
      insn::MachineInsnList insns;
      auto src = mapRValue(insns, load->getSource(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0, true);
      if (!src->isMemory() && core::analysis::isOffset(load->getSource())) {
        src = insn::buildMemOperand(src->getRegister(), insn::MachineOperand::OS_32Bit, 0);
      }

      auto dst = mapLValue(load->getTarget());
      // in case the target is a memory itself, we need to "help" mov
      if (src->isMemory() && dst->isMemory()) {
        auto eax = insn::buildRegOperand(insn::MachineOperand::OR_Eax);
        appendAll(insns, insn::buildMovTemplate(src, eax)->getInsns());
        src = eax;
      }
      appendAll(insns, insn::buildMovTemplate(src, dst)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class StoreMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isStoreInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto store = cast<core::StoreInsn>(insn);
      // expected
      // store $1,{v0,$0}
      insn::MachineInsnList insns;
      auto src = mapRValue(insns, store->getSource(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0, true);
      // dst is a little bit tricky .. handle cases
      auto dst = mapLValue(store->getTarget());
      // at this point we either hold a register or a memory location of the vN/tN
      if (core::analysis::isOffset(store->getTarget()) && dst->isMemory()) {
        auto ecx = insn::buildRegOperand(insn::MachineOperand::OR_Ecx);
        // ok, obtain the actual offset from that location via ecx helper
        appendAll(insns, insn::buildMovTemplate(dst, ecx)->getInsns());
        // fixup dst
        dst = ecx;
      }
      // regardless of case, it must still point into memory
      if (!dst->isMemory() && core::analysis::isOffset(store->getTarget()))
        dst = insn::buildMemOperand(dst->getRegister(), insn::MachineOperand::OS_32Bit, 0);
      // src is guarenteed to be in register form, thus no need to help out here as in load
      appendAll(insns, insn::buildMovTemplate(src, dst)->getInsns());
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  class AllocaMatcher : public RegAllocMatcher {
  public:
    using RegAllocMatcher::RegAllocMatcher;
    bool matches(const core::InsnPtr& insn) const override {
      return core::analysis::insn::isAllocaInsn(insn);
    }

    PatternResult generate(const core::InsnPtr& insn) const override {
      auto alloca = cast<core::AllocaInsn>(insn);
      insn::MachineInsnList insns;
      if (core::analysis::types::isArray(alloca->getVariable()->getType()) && !alloca->isConst()) {
        auto esp = insn::buildRegOperand(insn::MachineOperand::OR_Esp);
        auto ecx = insn::buildRegOperand(insn::MachineOperand::OR_Ecx);
        auto src = mapRValue(insns, alloca->getSize(), insn::MachineOperand::OR_Eax, insn::MachineOperand::OR_Xmm0, true);
        assert(src->isInt() && "alloca expects a size of type int");
        // dynamic allocas need to be handled here, rest are in static frame
        insns.push_back(insn::buildSubInsn(src, esp));
        // determine the base address of the array and store it in %ecx
        src = insn::buildMemOperand(insn::MachineOperand::OR_Esp, insn::MachineOperand::OS_32Bit, 0);
        insns.push_back(insn::buildLeaInsn(src, ecx));
        auto dst = mapLValue(alloca->getVariable());
        // now store in memory or dst reg
        appendAll(insns, insn::buildMovTemplate(ecx, dst)->getInsns());
      }
      return makeResult(std::make_shared<insn::TemplateInsn>(insns));
    }
  };

  RegAllocBackend::RegAllocBackend(const core::ProgramPtr& program) :
    Backend(program) {
    context = std::make_shared<RegAllocContext>(*this);
    matchers.push_back(makeMatcher<PlainAssignMatcher>(context));
    matchers.push_back(makeMatcher<LoadMatcher>(context));
    matchers.push_back(makeMatcher<StoreMatcher>(context));
    matchers.push_back(makeMatcher<AllocaMatcher>(context));
    matchers.push_back(makeMatcher<PushSpMatcher>(context));
    matchers.push_back(makeMatcher<PopSpMatcher>(context));
    matchers.push_back(makeMatcher<OffsetAssignMatcher>(context));
    matchers.push_back(makeMatcher<BinaryArithmeticShiftMatcher>(context));
    matchers.push_back(makeMatcher<BinaryArithmeticAddSubMatcher>(context));
    matchers.push_back(makeMatcher<BinaryArithmeticMulDivMatcher>(context));
    matchers.push_back(makeMatcher<BinaryLogicalJumpMatcher>(context));
    matchers.push_back(makeMatcher<BinaryLogicalAssignMatcher>(context));
    matchers.push_back(makeMatcher<UnaryAssignMatcher>(context));
    matchers.push_back(makeMatcher<FalseJumpMatcher>(context));
    matchers.push_back(makeMatcher<GotoMatcher>(context));
    matchers.push_back(makeMatcher<PushMatcher>(context));
    matchers.push_back(makeMatcher<CallMatcher>(context));
    matchers.push_back(makeMatcher<PopMatcher>(context));
    matchers.push_back(makeMatcher<ReturnMatcher>(context));
  }

  bool RegAllocBackend::convert() {
    std::stringstream ss;
    // as we have no globals, hop into text section
    ss << ".text" << std::endl;
    // generate all function which are not external
    for (const auto& fun : getProgram()->getFunctions()) {
      if (core::analysis::callgraph::isExternalFunction(fun)) continue;
      convert(ss, fun);
    }
    targetCode = ss.str();
    return true;
  }

  bool RegAllocBackend::convert(std::stringstream& ss, const core::FunctionPtr &fun) {
    std::string name = mangle::demangle(fun->getName());

    if (!core::analysis::callgraph::isAnonymousFunction(fun)) {
      ss << ".global " << name << std::endl;
      ss << ".func " << name << ", " << name << std::endl;
      // print out the type of the function as well
      fun->getType()->printTo(ss << "#");
      ss << std::endl;
    }

    // obtain a linear view of the block graph, as otherwise we cannot generate the insn
    auto bbs = core::analysis::controlflow::getLinearBasicBlockList(fun);
    // compute the new context
    context->setFrame(memory::getStackFrame(fun));
    if (getRegAlloc()) {
      auto insns = core::analysis::controlflow::getLinearInsnList(bbs);
      // use insns to compute liveness information
      core::analysis::worklist::InsnLiveness liveness;
      liveness.apply(insns.begin(), insns.end());
      // use the liveness to compute the registers
      context->setIntMapping(graph::color::getColorMappings(
        core::analysis::interference::getInterferenceGraph(fun, core::Type::TI_Int, liveness, insns),
        // use four colors, atm we map temporaries onto EBX, EDI and ESI & EDX as special case
        4));
      context->setLiveness(liveness);
    }
    bool first = true;
    // generate each block
    for (const auto& bb : bbs) {
      // generate the label
      ss << mangle::demangle(bb->getLabel()->getName()) << ":" << std::endl;
      // write the init frame?
      if (first) {
        first = false;
        // use any matcher to generate the init frame
        matchers[0]->buildFrameEntryTemplate()->printTo(ss);
      }
      unsigned skip = 0;
      for (const auto& insn : bb->getInsns()) {
        bool found = false;
        insn->printTo(ss << std::endl << "# ");
        if (skip) { --skip; continue; }
        // check if we have a matcher to generate machine code
        for (const auto& matcher : matchers) {
          if (!matcher->matches(insn)) continue;

          auto result = matcher->generate(insn);
          result.getInsn()->printTo(ss << std::endl);
          ss << std::endl;
          found = true;
          // preserve the skip counter
          skip = result.getCount() - 1;
          break;
        }
        assert(found && "no matcher was able to process the given insn");
      }
    }

    if (!core::analysis::callgraph::isAnonymousFunction(fun))
      ss << ".endfunc" << std::endl << std::endl;
    return true;
  }

  std::ostream& RegAllocBackend::printTo(std::ostream& stream) const {
    return stream << targetCode;
  }
}
}
