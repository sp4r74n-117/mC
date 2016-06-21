#include "backend/backend-insn.h"
#include "backend/backend-memory.h"
#include "core/analysis/analysis.h"
#include "core/arithmetic/arithmetic.h"

namespace backend {
namespace insn {
  namespace {
    std::string getInsnSuffix(MachineOperand::Bits bits) {
      switch (bits) {
      case MachineOperand::OS_8Bit:  return "b";
      case MachineOperand::OS_16Bit: return "w";
      case MachineOperand::OS_32Bit: return "l";
      default: return "";
      }
    }

    std::string getInsnName(MachineInsn::Opcode opcode) {
      switch (opcode) {
      case MachineInsn::OC_Ret:  return "ret";
      default: break;
      }
      assert(false && "unsupported no-ary opcode");
      return "";
    }

    std::string getInsnName(MachineInsn::Opcode opcode, const MachineOperandPtr& val) {
      std::string suffix = getInsnSuffix(val->getBits());
      switch (opcode) {
        case MachineInsn::OC_Push:            return "push" + suffix;
        case MachineInsn::OC_Pop:             return "pop" + suffix;
        case MachineInsn::OC_Call:            return "call";
        case MachineInsn::OC_Jmp:             return "jmp";
        case MachineInsn::OC_JmpEqual:        return "je";
        case MachineInsn::OC_JmpNotEqual:     return "jne";
        case MachineInsn::OC_JmpLessEqual:    return "jle";
  			case MachineInsn::OC_JmpLess:         return "jl";
        case MachineInsn::OC_JmpGreaterEqual: return "jge";
        case MachineInsn::OC_JmpGreater:      return "jg";
  			case MachineInsn::OC_JmpAbove:        return "ja";
        case MachineInsn::OC_JmpNotAbove:     return "jna";
        case MachineInsn::OC_JmpBelow:        return "jb";
        case MachineInsn::OC_JmpNotBelow:     return "jnb";
        case MachineInsn::OC_SetEqual:        return "sete";
        case MachineInsn::OC_SetNotEqual:     return "setne";
        case MachineInsn::OC_SetLess:         return "setl";
        case MachineInsn::OC_SetLessEqual:    return "setle";
        case MachineInsn::OC_SetGreater:      return "setg";
        case MachineInsn::OC_SetGreaterEqual: return "setge";
        case MachineInsn::OC_SetNotParity:    return "setnp";
        case MachineInsn::OC_SetParity:       return "setp";
        case MachineInsn::OC_SetAbove:        return "seta";
        case MachineInsn::OC_SetNotAbove:     return "setna";
        case MachineInsn::OC_SetBelow:        return "setb";
        case MachineInsn::OC_SetNotBelow:     return "setnb";
        case MachineInsn::OC_IDiv:            return "idiv" + suffix;
        case MachineInsn::OC_Neg:             return "neg";
        default: break;
      }
      assert(false && "unsupported unary opcode");
      return "";
    }

    std::string getInsnName(MachineInsn::Opcode opcode, const MachineOperandPtr& src, const MachineOperandPtr& dst) {
      std::string suffix = getInsnSuffix(src->isImmediate() ? dst->getBits() : src->getBits());
      switch (opcode) {
      case MachineInsn::OC_Mov:      return "mov" + suffix;
      case MachineInsn::OC_MovSs:    return "movss";
      case MachineInsn::OC_MovZbl:   return "movzbl";
      case MachineInsn::OC_MovEqual: return "cmove";
      case MachineInsn::OC_MovDw:    return "movd";
      case MachineInsn::OC_Sub:      return "sub" + suffix;
      case MachineInsn::OC_SubSs:    return "subss";
      case MachineInsn::OC_Add:      return "add" + suffix;
      case MachineInsn::OC_AddSs:    return "addss";
      case MachineInsn::OC_Cmp:      return "cmp" + suffix;
      case MachineInsn::OC_UComIss:  return "ucomiss";
      case MachineInsn::OC_IMul:     return "imul" + suffix;
      case MachineInsn::OC_MulSs:    return "mulss";
      case MachineInsn::OC_DivSs:    return "divss";
      case MachineInsn::OC_Xor:      return "xor" + suffix;
      case MachineInsn::OC_XorSs:    return "xorps";
      case MachineInsn::OC_Lea:      return "lea";
      case MachineInsn::OC_Sal:      return "sal" + suffix;
      case MachineInsn::OC_Sar:      return "sar" + suffix;
      default: break;
      }
      assert(false && "unsupported binary opcode");
      return "";
    }

    std::string getRegName(MachineOperand::Register reg, MachineOperand::Bits bits) {
      switch (reg) {
      case MachineOperand::OR_Eax:
        if (bits == MachineOperand::OS_32Bit) return "%eax";
        if (bits == MachineOperand::OS_16Bit) return "%ax";
        if (bits == MachineOperand::OS_8Bit)  return "%al";
        break;
      case MachineOperand::OR_Ebx:
        if (bits == MachineOperand::OS_32Bit) return "%ebx";
        if (bits == MachineOperand::OS_16Bit) return "%bx";
        if (bits == MachineOperand::OS_8Bit)  return "%bl";
        break;
      case MachineOperand::OR_Ecx:
        if (bits == MachineOperand::OS_32Bit) return "%ecx";
        if (bits == MachineOperand::OS_16Bit) return "%cx";
        if (bits == MachineOperand::OS_8Bit)  return "%cl";
        break;
      case MachineOperand::OR_Edx:
        if (bits == MachineOperand::OS_32Bit) return "%edx";
        if (bits == MachineOperand::OS_16Bit) return "%dx";
        if (bits == MachineOperand::OS_8Bit)  return "%dl";
        break;
      case MachineOperand::OR_Edi:
        if (bits == MachineOperand::OS_32Bit) return "%edi";
        if (bits == MachineOperand::OS_16Bit) return "%di";
        break;
      case MachineOperand::OR_Esi:
        if (bits == MachineOperand::OS_32Bit) return "%esi";
        if (bits == MachineOperand::OS_16Bit) return "%si";
        break;
      case MachineOperand::OR_Eip:  return "%eip";
      case MachineOperand::OR_Ebp:  return "%ebp";
      case MachineOperand::OR_Esp:  return "%esp";
      case MachineOperand::OR_Xmm0: return "%xmm0";
      case MachineOperand::OR_Xmm1: return "%xmm1";
      case MachineOperand::OR_Xmm2: return "%xmm2";
      default: break;
      }

      assert(false && "invalid register/bits combination");
      return "";
    }

    MachineOperand::Type getRegType(MachineOperand::Register reg) {
      MachineOperand::Type type;
      switch (reg) {
      case MachineOperand::OR_Xmm0:
      case MachineOperand::OR_Xmm1:
      case MachineOperand::OR_Xmm2: type = MachineOperand::OT_Float; break;
      default:                      type = MachineOperand::OT_Int; break;
      }
      return type;
    }

    void assertMov(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
      // http://x86.renejeschke.de/html/file_module_x86_id_176.html
      assert(src->getBits() == dst->getBits() &&
        "mov required source and destination operand of same bit size");
      assert(!dst->isImmediate() && "destination operand must not be an immediate");
    }

    void assertMovSs(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
      // http://x86.renejeschke.de/html/file_module_x86_id_205.html
      // mnemonic is as follows:
      // MOVSS xmm1, xmm2/m32
      // MOVSS xmm2/m32, xmm1
      if (src->isFloat()) {
        assert(src->isRegister() && "source operand must xmm");
        assert(((dst->isFloat() && dst->isRegister()) || dst->isMemory()) &&
          "destination operand must be xmm/m32");
      }

      if (dst->isFloat()) {
        assert(dst->isRegister() && "destination operand must xmm");
        assert(((src->isFloat() && src->isRegister()) || src->isMemory()) &&
          "source operand must be xmm/m32");
      }
    }

    void assertMovZbl(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
      assert(src->getBits() == MachineOperand::OS_8Bit &&
             dst->getBits() == MachineOperand::OS_32Bit &&
             "movzbl requires 8-bit source and 32-bit destination");
    }

    void assertSetCc(const MachineOperandPtr& dst) {
      assert(dst->getBits() == MachineOperand::OS_8Bit && "setcc requires r/m8");
    }

    void assertLabel(const MachineOperandPtr& label) {
      assert(label->isLocation() && "label requires a location operand");
    }

    void assertLea(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
      // mnemonic expects lea as follows:
      // LEA r{32,16},m
      assert(dst->isRegister() &&
             (dst->getBits() == MachineOperand::OS_16Bit || dst->getBits() == MachineOperand::OS_32Bit) &&
             "lea requires a r{16,32} destination");
      assert(src->isMemory() && "lea requires memory location as source");
    }

    void assertJcc(const MachineOperandPtr& target) {
      assert(target->isLocation() && "jcc requires a location operand");
    }

    void assertSaCc(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
      // mnemonic expects sa{r,l} as follows:
      // SAL r/m{8,16,32},imm8
      assert((dst->isRegister() || dst->isMemory()) &&
        "sacc requires a register or memory location as destination operand");
      assert(src->isImmediate() && src->isInt() &&
        "sacc requires an immediate as source operand");
      // also check the range
      auto imm = static_cast<unsigned>(src->getImmediate());
      assert(imm <= 0xFF && "sacc requires an immediate 0..0xFF as source operand");
    }

    void assertMovDw(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
      // mnemonic expects movd as follows:
      // MOVD mm, r/m32
      // MOVD r/m32, mm
      assert(((dst->isRegister() && dst->isFloat()) ||
              (src->isRegister() && src->isFloat())) && "movd requires xmm as source or destination");
    }
  }

  bool MachineOperand::operator==(const MachineOperand& other) const {
    return op == other.op && type == other.type && reg == other.reg &&
           bits == other.bits && value == other.value && offset == other.offset &&
           loc == other.loc;
  }

  bool MachineOperand::operator!=(const MachineOperand& other) const {
    return !(*this == other);
  }

  std::ostream& MachineOperand::printTo(std::ostream& stream) const {
    switch (op) {
    case OC_Mem:
        // always use the 32bit name, bits has only affect on access!
        stream << getOffset() << "(" << getRegName(reg, OS_32Bit) << ")";
        break;
    case OC_Loc:
        stream << getLocation();
        break;
    case OC_Reg:
        stream << getRegName(reg, bits);
        break;
    case OC_Imm:
          char buffer[64];
          union { uint32_t i; float f; } f2i;
          if (getType() == OT_Int) {
            unsigned int value = getImmediate();
            std::sprintf(buffer, "$0x%x", value);
          } else {
            f2i.f = getImmediate();
            // encode the float as hex value in IEEE-754 format
            std::sprintf(buffer, "$0x%x", f2i.i);
          }
          stream << std::string(buffer);
        break;
    default:
        assert(false && "unsupported operand type");
        break;
    }
    return stream;
  }

  std::ostream& MachineInsn::printTo(std::ostream& stream) const {
    switch (op) {
    case OC_Mov:
    case OC_Sub:
    case OC_Add:
    case OC_Cmp:
    case OC_IMul:
    case OC_MulSs:
    case OC_DivSs:
    case OC_UComIss:
    case OC_AddSs:
    case OC_SubSs:
    case OC_Xor:
    case OC_XorSs:
    case OC_MovSs:
    case OC_MovZbl:
    case OC_MovEqual:
    case OC_MovDw:
    case OC_Lea:
    case OC_Sal:
    case OC_Sar:
        stream << getInsnName(getOpcode(), getRhs1(), getRhs2());
        getRhs1()->printTo(stream << " ");
        getRhs2()->printTo(stream << ",");
        break;
    case OC_Push:
    case OC_Pop:
    case OC_Call:
    case OC_Jmp:
    case OC_JmpEqual:
    case OC_JmpNotEqual:
    case OC_JmpLessEqual:
    case OC_JmpLess:
    case OC_JmpGreaterEqual:
    case OC_JmpGreater:
    case OC_JmpAbove:
    case OC_JmpNotAbove:
    case OC_JmpBelow:
    case OC_JmpNotBelow:
    case OC_SetEqual:
    case OC_SetNotEqual:
    case OC_SetLess:
    case OC_SetLessEqual:
    case OC_SetGreater:
    case OC_SetGreaterEqual:
    case OC_SetNotParity:
    case OC_SetParity:
    case OC_SetAbove:
    case OC_SetNotAbove:
    case OC_SetBelow:
    case OC_SetNotBelow:
    case OC_IDiv:
    case OC_Neg:
        stream << getInsnName(getOpcode(), getRhs1());
        getRhs1()->printTo(stream << " ");
        break;
    case OC_Label:
        getRhs1()->printTo(stream);
        stream << ":";
        break;
    case OC_Ret:
        stream << getInsnName(getOpcode());
        break;
    default:
        assert(false && "unsupported insn opcode");
        break;
    }
    return stream;
  }

  std::ostream& TemplateInsn::printTo(std::ostream& stream) const {
    bool first = true;
    for (const auto& insn : getInsns()) {
      if (first) first = false;
      else       stream << std::endl;
      insn->printTo(stream);
    }
    return stream;
  }

  MachineOperandPtr buildRegOperand(MachineOperand::Register reg) {
    return buildRegOperand(reg, MachineOperand::OS_32Bit);
  }

  MachineOperandPtr buildRegOperand(MachineOperand::Register reg, MachineOperand::Bits bits) {
    // deduce the type by looking at the register itself
    MachineOperand::Type type = getRegType(reg);
    if (type == MachineOperand::OT_Float) {
      // in case of float, additional constraints arise
      assert(bits == MachineOperand::OS_32Bit &&
        "sse register can only be referenced by single-precision 32-bit float");
    }
    return std::make_shared<MachineOperand>(type, reg, bits);
  }

  MachineOperandPtr buildRegOperand(const MachineOperandPtr& reg, MachineOperand::Bits bits) {
    assert(reg->isRegister() && "cannot build a reg operand from a different one");
    return buildRegOperand(reg->getRegister(), bits);
  }

  MachineOperandPtr buildLocOperand(const std::string& location) {
    return std::make_shared<MachineOperand>(location);
  }

  MachineOperandPtr buildMemOperand(MachineOperand::Register reg, MachineOperand::Bits bits, int offset) {
    MachineOperand::Type type = getRegType(reg);
    assert(type == MachineOperand::OT_Int &&
      "sse register must not be used to reference a memory location");
    return std::make_shared<MachineOperand>(reg, bits, offset);
  }

  MachineOperandPtr buildMemOperand(int offset) {
    return buildMemOperand(MachineOperand::OR_Ebp, MachineOperand::OS_32Bit, offset);
  }

  MachineOperandPtr buildImmOperand(int value) {
    return std::make_shared<MachineOperand>(MachineOperand::OT_Int, value);
  }

  MachineOperandPtr buildImmOperand(float value) {
    return std::make_shared<MachineOperand>(MachineOperand::OT_Float, value);
  }

  MachineOperandPtr buildImmOperand(const core::ValuePtr& value) {
    assert(core::analysis::isConstant(value) && "immeditate builder must be used with a constant");

    if (core::analysis::isIntConstant(value))
      return buildImmOperand(core::arithmetic::getValue<int>(value));
    else
      return buildImmOperand(core::arithmetic::getValue<float>(value));
  }

  MachineInsnPtr buildMovInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertMov(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_Mov, src, dst);
  }

  MachineInsnPtr buildMovSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertMovSs(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_MovSs, src, dst);
  }

  MachineInsnPtr buildMovZblInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertMovZbl(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_MovZbl, src, dst);
  }

  MachineInsnPtr buildMovEqualInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertMov(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_MovEqual, src, dst);
  }

  MachineInsnPtr buildMovDwInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertMovDw(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_MovDw, src, dst);
  }

  MachineInsnPtr buildSubInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_Sub, src, dst);
  }

  MachineInsnPtr buildSubSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_SubSs, src, dst);
  }

  MachineInsnPtr buildAddInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_Add, src, dst);
  }

  MachineInsnPtr buildAddSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_AddSs, src, dst);
  }

  MachineInsnPtr buildIMulInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_IMul, src, dst);
  }

  MachineInsnPtr buildIDivInsn(const MachineOperandPtr& src) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_IDiv, src);
  }

  MachineInsnPtr buildMulSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_MulSs, src, dst);
  }

  MachineInsnPtr buildDivSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_DivSs, src, dst);
  }

  MachineInsnPtr buildLabelInsn(const MachineOperandPtr& label) {
    assertLabel(label);
    return std::make_shared<MachineInsn>(MachineInsn::OC_Label, label);
  }

  MachineInsnPtr buildLeaInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertLea(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_Lea, src, dst);
  }

  MachineInsnPtr buildRetInsn() {
    return std::make_shared<MachineInsn>(MachineInsn::OC_Ret);
  }

  MachineInsnPtr buildCallInsn(const MachineOperandPtr& callee) {
    // TODO checsk
    return std::make_shared<MachineInsn>(MachineInsn::OC_Call, callee);
  }

  MachineInsnPtr buildJmpInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_Jmp, target);
  }

  MachineInsnPtr buildJmpEqualInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpEqual, target);
  }

  MachineInsnPtr buildJmpNotEqualInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpNotEqual, target);
  }

  MachineInsnPtr buildJmpLessEqualInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpLessEqual, target);
  }

  MachineInsnPtr buildJmpLessInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpLess, target);
  }

  MachineInsnPtr buildJmpGreaterEqualInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpGreaterEqual, target);
  }

  MachineInsnPtr buildJmpGreaterInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpGreater, target);
  }

  MachineInsnPtr buildJmpAboveInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpAbove, target);
  }

  MachineInsnPtr buildJmpNotAboveInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpNotAbove, target);
  }

  MachineInsnPtr buildJmpBelowInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpBelow, target);
  }

  MachineInsnPtr buildJmpNotBelowInsn(const MachineOperandPtr& target) {
    assertJcc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_JmpNotBelow, target);
  }

  MachineInsnPtr buildCmpInsn(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_Cmp, lhs, rhs);
  }

  MachineInsnPtr buildUComIssInsn(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_UComIss, lhs, rhs);
  }

  MachineInsnPtr buildNotInsn(const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_Xor, dst, dst);
  }

  MachineInsnPtr buildXorInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_Xor, src, dst);
  }

  MachineInsnPtr buildXorSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_XorSs, src, dst);
  }

  MachineInsnPtr buildNegInsn(const MachineOperandPtr& dst) {
    // TODO checks
    return std::make_shared<MachineInsn>(MachineInsn::OC_Neg, dst);
  }

  MachineInsnPtr buildSetEqualInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetEqual, target);
  }

  MachineInsnPtr buildSetNotEqualInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetNotEqual, target);
  }

  MachineInsnPtr buildSetLessEqualInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetLessEqual, target);
  }

  MachineInsnPtr buildSetLessInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetLess, target);
  }

  MachineInsnPtr buildSetGreaterEqualInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetGreaterEqual, target);
  }

  MachineInsnPtr buildSetGreaterInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetGreater, target);
  }

  MachineInsnPtr buildSetParityInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetParity, target);
  }

  MachineInsnPtr buildSetNotParityInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetNotParity, target);
  }

  MachineInsnPtr buildSetAboveInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetAbove, target);
  }

  MachineInsnPtr buildSetNotAboveInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetNotAbove, target);
  }

  MachineInsnPtr buildSetBelowInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetBelow, target);
  }

  MachineInsnPtr buildSetNotBelowInsn(const MachineOperandPtr& target) {
    assertSetCc(target);
    return std::make_shared<MachineInsn>(MachineInsn::OC_SetNotBelow, target);
  }

  MachineInsnPtr buildSalInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertSaCc(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_Sal, src, dst);
  }

  MachineInsnPtr buildSarInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    assertSaCc(src, dst);
    return std::make_shared<MachineInsn>(MachineInsn::OC_Sar, src, dst);
  }

  void TemplateInsn::append(const TemplateInsnPtr& templateInsn, const MachineInsnPtr& insn) {
    templateInsn->getInsns().push_back(insn);
  }

  void TemplateInsn::append(const TemplateInsnPtr& templateInsn, const TemplateInsnPtr& insn) {
    templateInsn->getInsns().insert(templateInsn->getInsns().end(), insn->getInsns().begin(), insn->getInsns().end());
  }

  void TemplateInsn::prepend(const TemplateInsnPtr& templateInsn, const MachineInsnPtr& insn) {
    templateInsn->getInsns().insert(templateInsn->getInsns().begin(), insn);
  }

  void TemplateInsn::prepend(const TemplateInsnPtr& templateInsn, const TemplateInsnPtr& insn) {
    templateInsn->getInsns().insert(templateInsn->getInsns().begin(), insn->getInsns().begin(), insn->getInsns().end());
  }

  TemplateInsnPtr buildFrameEntryTemplate(const memory::StackFramePtr& frame) {
    MachineInsnList insns;
    // we generate a standard entry signature
    // push %ebp
    // mov %esp, %ebp
    // sub <amount>, %esp
    auto ebp = buildRegOperand(MachineOperand::OR_Ebp, MachineOperand::OS_32Bit);
    auto esp = buildRegOperand(MachineOperand::OR_Esp, MachineOperand::OS_32Bit);

    auto push = buildPushTemplate(ebp);
    insns.insert(insns.end(), push->getInsns().begin(), push->getInsns().end());
    insns.push_back(buildMovInsn(esp, ebp));
    if (frame->getNumOfBytesFrame()) {
      // allocate the specified number of bytes on the stack
      insns.push_back(buildSubInsn(buildImmOperand((int) frame->getNumOfBytesFrame()), esp));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildFrameLeaveTemplate(const memory::StackFramePtr& frame) {
    MachineInsnList insns;
    // we generate the standard leave signature
    // add <amount>, %esp
    // mov %ebp, %esp
    // pop %ebp
    auto ebp = buildRegOperand(MachineOperand::OR_Ebp, MachineOperand::OS_32Bit);
    auto esp = buildRegOperand(MachineOperand::OR_Esp, MachineOperand::OS_32Bit);
    if (frame->getNumOfBytesFrame()) {
      // allocate the specified number of bytes on the stack
      insns.push_back(buildAddInsn(buildImmOperand((int) frame->getNumOfBytesFrame()), esp));
    }
    insns.push_back(buildMovInsn(ebp, esp));
    // generate the pop template
    auto pop = buildPopTemplate(ebp);
    insns.insert(insns.end(), pop->getInsns().begin(), pop->getInsns().end());
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildPushTemplate(const MachineOperandPtr& val) {
    assert((val->isRegister() || val->isMemory() || val->isImmediate()) &&
      "push expects are register, memory location or immediate");

    MachineInsnList insns;
    if (val->isInt() || val->isImmediate()) {
      insns.push_back(std::make_shared<MachineInsn>(MachineInsn::OC_Push, val));
    } else {
      // model a single-precision push as
      // sub $4,%esp
      // movss %xmm0,$0(%esp)
      auto esp = buildRegOperand(MachineOperand::OR_Esp, MachineOperand::OS_32Bit);
      insns.push_back(buildSubInsn(buildImmOperand(4), esp));
      // mark the stack location as in in order to generate movss
      insns.push_back(buildMovSsInsn(val, buildMemOperand(MachineOperand::OR_Esp, MachineOperand::OS_32Bit, 0)));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildPopTemplate(const MachineOperandPtr& val) {
    assert((val->isRegister() || val->isMemory() || val->isImmediate()) &&
      "pop expects are register, memory location or immediate");

    MachineInsnList insns;
    if (val->isInt() && val->isRegister()) {
      insns.push_back(std::make_shared<MachineInsn>(MachineInsn::OC_Pop, val));
    } else if (val->isInt() && val->isImmediate()) {
      insns.push_back(buildAddInsn(val, buildRegOperand(MachineOperand::Register::OR_Esp, MachineOperand::OS_32Bit)));
    } else {
      // model a single-precision pop as
      // movss $0(%esp),%xmm0
      // add $4,%esp
      auto esp = buildRegOperand(MachineOperand::OR_Esp, MachineOperand::OS_32Bit);
      // mark the stack location as in in order to generate movss
      insns.push_back(buildMovInsn(buildMemOperand(MachineOperand::OR_Esp, MachineOperand::OS_32Bit, 0), val));
      insns.push_back(buildAddInsn(buildImmOperand(4), esp));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildMovTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    // fast path?
    if (*src == *dst) return std::make_shared<TemplateInsn>(insns);
    // slow-path ...
    if ((src->isFloat() && src->isRegister()) || (dst->isFloat() && dst->isRegister())) {
      // in case src is an immediate, use movd for this purpose
      if (src->isImmediate()) {
        auto eax = buildRegOperand(MachineOperand::OR_Eax);
        insns.push_back(buildMovInsn(src, eax));
        insns.push_back(buildMovDwInsn(eax, dst));
      } else {
        // if once of the operands is xmm, redirect to movss
        insns.push_back(buildMovSsInsn(src, dst));
      }
    } else if (src->getBits() == dst->getBits()) {
      // in case the bits are the same, assume generic mov
      insns.push_back(buildMovInsn(src, dst));
    } else {
      // assume zbl
      insns.push_back(buildMovZblInsn(src, dst));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildAddTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    if (src->isFloat() || dst->isFloat()) insns.push_back(buildAddSsInsn(src, dst));
    else insns.push_back(buildAddInsn(src, dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildSubTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    if (src->isFloat() || dst->isFloat()) insns.push_back(buildSubSsInsn(src, dst));
    else insns.push_back(buildSubInsn(src, dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildCmpTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs) {
    // TODO checks
    MachineInsnList insns;
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildUComIssInsn(lhs, rhs));
    else insns.push_back(buildCmpInsn(lhs, rhs));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildMulTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    // Explanation:
    // The source operand is multiplied by the value in the EAX register and
    // the product is stored in the DX:EAX registers.
    // In case of float: MULSS xmm2/m32, xmm1 (where xmm2 = src, xmm1 = dst)
    MachineInsnList insns;
    if (src->isFloat() || dst->isFloat()) insns.push_back(buildMulSsInsn(src, dst));
    else insns.push_back(buildIMulInsn(src, dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildDivTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst) {
    // TODO checks
    MachineInsnList insns;
    if (src->isFloat() || dst->isFloat()) {
      insns.push_back(buildDivSsInsn(src, dst));
    } else {
      // http://x86.renejeschke.de/html/file_module_x86_id_137.html
      // IDIV r/m32, Signed divide EDX:EAX by r/m32, with result stored in EAX = Quotient, EDX = Remainder.
      auto eax = buildRegOperand(MachineOperand::OR_Eax, MachineOperand::OS_32Bit);
      auto edx = buildRegOperand(MachineOperand::OR_Edx, MachineOperand::OS_32Bit);
      if (dst->getRegister() != MachineOperand::OR_Eax)
        // move it into %eax first
        insns.push_back(buildMovInsn(dst, eax));
      // clear out the extended EDX part
      insns.push_back(buildXorInsn(edx, edx));
      // carry out the division
      insns.push_back(buildIDivInsn(src));
      // and move into result again
      if (dst->getRegister() != MachineOperand::OR_Eax)
        insns.push_back(buildMovInsn(eax, dst));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildNegTemplate(const MachineOperandPtr& dst) {
    // TODO checks
    MachineInsnList insns;
    if (dst->isInt()) {
      insns.push_back(buildNegInsn(dst));
    } else {
      auto tmp0 = buildRegOperand(dst->getRegister() == MachineOperand::OR_Xmm0 ?
        MachineOperand::OR_Xmm1 : MachineOperand::OR_Xmm0, MachineOperand::OS_32Bit);
      appendAll(insns, buildMovTemplate(buildImmOperand(static_cast<int>(0x80000000)), tmp0)->getInsns());
      insns.push_back(buildXorSsInsn(tmp0, dst));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildNotTemplate(const MachineOperandPtr& dst) {
    MachineInsnList insns;
    auto al = insn::buildRegOperand(insn::MachineOperand::OR_Eax, insn::MachineOperand::OS_8Bit);
    auto eax = insn::buildRegOperand(insn::MachineOperand::OR_Eax, insn::MachineOperand::OS_32Bit);
    // generate the compare
    appendAll(insns, insn::buildCmpTemplate(buildImmOperand(0), dst)->getInsns());
    // generate the correct compare
    insns.push_back(buildSetEqualInsn(al));
    insns.push_back(insn::buildMovZblInsn(al, eax));
    // and move to the target location afterwards
    insns.push_back(insn::buildMovInsn(eax, dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    // CAUTION: if (b == 7.7) where typeof(b) == float
    // the problem is that 7.7 is double, b is upconverted and the following
    // is generated by gcc:
    // 8048607:	f3 0f 5a 45 08       	cvtss2sd 0x8(%ebp),%xmm0
    // 804860c:	66 0f 2e 05 30 88 04 	ucomisd 0x8048830,%xmm0
    // this will result in ZF beeing zero and thus the comparions will fail!
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isInt()) {
      insns.push_back(insn::buildSetEqualInsn(dst));
    } else {
      // safely use ecx here as it does not interfere with allocated gpr regs
      auto ecx = insn::buildRegOperand(insn::MachineOperand::OR_Ecx, insn::MachineOperand::OS_32Bit);
      auto tmp = buildRegOperand(dst, MachineOperand::OS_32Bit);
      insns.push_back(insn::buildSetNotParityInsn(buildRegOperand(ecx, MachineOperand::OS_8Bit)));
      insns.push_back(insn::buildXorInsn(tmp, tmp));
      // compare once again
      appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
      // conditionally move %cl (which holds NP) into dst
      insns.push_back(insn::buildMovEqualInsn(ecx, tmp));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildNotEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    // CAUTION: if (b == 7.7) where typeof(b) == float
    // the problem is that 7.7 is double, b is upconverted and the following
    // is generated by gcc:
    // 8048607:	f3 0f 5a 45 08       	cvtss2sd 0x8(%ebp),%xmm0
    // 804860c:	66 0f 2e 05 30 88 04 	ucomisd 0x8048830,%xmm0
    // this will result in ZF beeing zero and thus the comparions will fail!
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isInt()) {
      insns.push_back(insn::buildSetNotEqualInsn(dst));
    } else {
      // safely use ecx here as it does not interfere with allocated gpr regs
      auto ecx = insn::buildRegOperand(MachineOperand::OR_Ecx, MachineOperand::OS_32Bit);
      auto tmp = buildRegOperand(dst, MachineOperand::OS_32Bit);
      insns.push_back(insn::buildSetParityInsn(buildRegOperand(ecx, MachineOperand::OS_8Bit)));
      // optimize movl $1, dst with straight xor
      insns.push_back(insn::buildXorInsn(tmp, tmp));
      appendAll(insns, insn::buildAddTemplate(insn::buildImmOperand(1), dst)->getInsns());
      // compare once again
      appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
      // conditionally move %dl (which holds NP) into dst
      insns.push_back(insn::buildMovEqualInsn(ecx, tmp));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildLessEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildSetNotAboveInsn(dst));
    else insns.push_back(buildSetLessEqualInsn(dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildLessTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildSetBelowInsn(dst));
    else insns.push_back(buildSetLessInsn(dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildGreaterEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildSetNotBelowInsn(dst));
    else insns.push_back(buildSetGreaterEqualInsn(dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildGreaterTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildSetAboveInsn(dst));
    else insns.push_back(buildSetGreaterInsn(dst));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildJmpEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target) {
    MachineInsnList insns;
    // CAUTION: if (b == 7.7) where typeof(b) == float
    // the problem is that 7.7 is double, b is upconverted and the following
    // is generated by gcc:
    // 8048607:	f3 0f 5a 45 08       	cvtss2sd 0x8(%ebp),%xmm0
    // 804860c:	66 0f 2e 05 30 88 04 	ucomisd 0x8048830,%xmm0
    // this will result in ZF beeing zero and thus the comparions will fail!
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isInt()) {
      insns.push_back(insn::buildJmpEqualInsn(target));
    } else {
      // safely use eax & ecx here as it does not interfere with allocated gpr regs
      auto eax = insn::buildRegOperand(MachineOperand::OR_Eax, MachineOperand::OS_32Bit);
      auto ecx = insn::buildRegOperand(MachineOperand::OR_Ecx, MachineOperand::OS_32Bit);
      insns.push_back(insn::buildSetNotParityInsn(buildRegOperand(eax, MachineOperand::OS_8Bit)));
      insns.push_back(insn::buildXorInsn(ecx, ecx));
      // compare once again
      appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
      // conditionally move %eax (which holds NP) into %ecx
      insns.push_back(insn::buildMovEqualInsn(eax, ecx));
      insns.push_back(insn::buildJmpEqualInsn(target));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildJmpNotEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target) {
    MachineInsnList insns;
    // CAUTION: if (b == 7.7) where typeof(b) == float
    // the problem is that 7.7 is double, b is upconverted and the following
    // is generated by gcc:
    // 8048607:	f3 0f 5a 45 08       	cvtss2sd 0x8(%ebp),%xmm0
    // 804860c:	66 0f 2e 05 30 88 04 	ucomisd 0x8048830,%xmm0
    // this will result in ZF beeing zero and thus the comparions will fail!
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isInt()) {
      insns.push_back(insn::buildJmpNotEqualInsn(target));
    } else {
      // safely use eax & ecx here as it does not interfere with allocated gpr regs
      auto eax = insn::buildRegOperand(MachineOperand::OR_Eax, MachineOperand::OS_32Bit);
      auto ecx = insn::buildRegOperand(MachineOperand::OR_Ecx, MachineOperand::OS_32Bit);
      insns.push_back(insn::buildSetParityInsn(buildRegOperand(eax, MachineOperand::OS_8Bit)));
      // optimize movl $1, %ecx with straight xor
      insns.push_back(insn::buildXorInsn(ecx, ecx));
      appendAll(insns, insn::buildAddTemplate(insn::buildImmOperand(1), ecx)->getInsns());
      // compare once again
      appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
      // conditionally move %eax (which holds NP) into %ecx
      insns.push_back(insn::buildMovEqualInsn(eax, ecx));
      insns.push_back(insn::buildXorInsn(ecx, ecx));
      insns.push_back(insn::buildJmpNotEqualInsn(target));
    }
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildJmpLessEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildJmpNotAboveInsn(target));
    else insns.push_back(buildJmpLessEqualInsn(target));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildJmpLessTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildJmpBelowInsn(target));
    else insns.push_back(buildJmpLessInsn(target));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildJmpGreaterEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildJmpNotBelowInsn(target));
    else insns.push_back(buildJmpGreaterEqualInsn(target));
    return std::make_shared<TemplateInsn>(insns);
  }

  TemplateInsnPtr buildJmpGreaterTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target) {
    MachineInsnList insns;
    appendAll(insns, insn::buildCmpTemplate(lhs, rhs)->getInsns());
    if (lhs->isFloat() || rhs->isFloat()) insns.push_back(buildJmpAboveInsn(target));
    else insns.push_back(buildJmpGreaterInsn(target));
    return std::make_shared<TemplateInsn>(insns);
  }
}
}
