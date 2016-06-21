#pragma once
#include "core/core.h"

namespace backend {
namespace memory {
	class StackFrame;
	typedef Ptr<StackFrame> StackFramePtr;
}
namespace insn {
  class MachineOperand;
  typedef Ptr<MachineOperand> MachineOperandPtr;
  class MachineInsn;
  typedef Ptr<MachineInsn> MachineInsnPtr;
  class TemplateInsn;
  typedef Ptr<TemplateInsn> TemplateInsnPtr;

  typedef PtrList<MachineInsn> MachineInsnList;

  class MachineOperand : public Printable {
  public:
    enum Opcode   { OC_Reg, OC_Mem, OC_Loc, OC_Imm };
    enum Register {
      // x86 standard registers of type int
      OR_Eax, OR_Ebx, OR_Ecx, OR_Edx, OR_Ebp, OR_Esp, OR_Edi, OR_Esi, OR_Eip,
      // sse related registers
      OR_Xmm0, OR_Xmm1, OR_Xmm2,
      // used for placeholder was specified or e.g. OC_Imm
      OR_Undefined
    };
    enum Bits {
      // modifies the access of a register e.g %eax is used as %al
      OS_32Bit, OS_16Bit, OS_8Bit, OS_Undefined
    };
    enum Type {
      // describes the abstract underlying type
      OT_Int, OT_Float, OT_Undefined
    };

    MachineOperand(Type type, Register reg, Bits bits) :
      op(OC_Reg), type(type), reg(reg), bits(bits), value(0.0f), offset(0)
    { }
    MachineOperand(Register reg, Bits bits, int offset) :
      op(OC_Mem), type(OT_Int), reg(reg), bits(bits), value(0.0f), offset(offset)
    { }
    MachineOperand(Type type, float value) :
      op(OC_Imm), type(type), reg(OR_Undefined), bits(OS_32Bit), value(value), offset(0)
    { }
    MachineOperand(const std::string& location) :
      op(OC_Loc), type(OT_Undefined), reg(OR_Undefined), bits(OS_Undefined), value(0), offset(0), loc(location)
    { }
    Opcode getOpcode() const { return op; }
    Type getType() const { return type; }
    Register getRegister() const { return reg; }
    float getImmediate() const { return value; }
    int getOffset() const { return offset; }
    Bits getBits() const { return bits; }
    const std::string& getLocation() const { return loc; }
    bool isMemory() const { return op == OC_Mem; }
    bool isRegister() const { return op == OC_Reg; }
    bool isImmediate() const { return op == OC_Imm; }
    bool isInt() const { return type == OT_Int; }
    bool isFloat() const { return type == OT_Float; }
		bool isLocation() { return op == OC_Loc; }
		bool operator==(const MachineOperand& other) const;
		bool operator!=(const MachineOperand& other) const;
    std::ostream& printTo(std::ostream& stream) const override;
  private:
    Opcode op;
    Type type;
    Register reg;
    Bits bits;
    float value;
    int offset;
    std::string loc;
  };

	MachineOperandPtr buildRegOperand(MachineOperand::Register reg);
  MachineOperandPtr buildRegOperand(MachineOperand::Register reg, MachineOperand::Bits bits);
  MachineOperandPtr buildRegOperand(const MachineOperandPtr& reg, MachineOperand::Bits bits);
  MachineOperandPtr buildMemOperand(MachineOperand::Register reg, MachineOperand::Bits bits, int offset);
	MachineOperandPtr buildMemOperand(int offset);
  MachineOperandPtr buildLocOperand(const std::string& location);
  MachineOperandPtr buildImmOperand(int value);
  MachineOperandPtr buildImmOperand(float value);
  MachineOperandPtr buildImmOperand(const core::ValuePtr& value);

  class MachineInsn : public Printable {
  public:
    enum Opcode {
      // unconditional generic mov http://x86.renejeschke.de/html/file_module_x86_id_176.html
      OC_Mov, OC_MovSs, OC_MovZbl, OC_MovDw,
      // pusl & popl for stack management
      OC_Push, OC_Pop,
      // arithmetic ops for sse and gpr
      OC_Sub, OC_Add, OC_IMul, OC_IDiv, OC_SubSs,
			OC_AddSs, OC_MulSs, OC_DivSs, OC_Sal, OC_Sar,
      // bit manipulation
      OC_Xor, OC_XorSs, OC_Neg,
      // test functions which affect CF, OF, SF, ZF, AF, and PF
      // http://x86.renejeschke.de/html/file_module_x86_id_35.html
      // http://x86.renejeschke.de/html/file_module_x86_id_40.html
      OC_Cmp, OC_UComIss,
      // unconditional control branches
      OC_Ret, OC_Call, OC_Jmp,
      // conditional control branches
      OC_JmpEqual, OC_JmpNotEqual, OC_JmpLessEqual,
			OC_JmpLess, OC_JmpGreaterEqual, OC_JmpGreater,
			OC_JmpAbove, OC_JmpNotAbove, OC_JmpBelow, OC_JmpNotBelow,
      // conditional set insn
      OC_SetEqual, OC_SetNotEqual, OC_SetLessEqual,
      OC_SetLess, OC_SetGreaterEqual, OC_SetGreater,
      OC_SetNotParity, OC_SetParity, OC_SetAbove,
      OC_SetNotAbove,  OC_SetBelow, OC_SetNotBelow,
      // conditional moves
      OC_MovEqual,
			// misc
			OC_Label, OC_Lea
    };
    MachineInsn(Opcode op) :
      op(op)
    { }
    MachineInsn(Opcode op, const MachineOperandPtr& rhs1) :
      op(op), rhs1(rhs1)
    { }
    MachineInsn(Opcode op, const MachineOperandPtr& rhs1, const MachineOperandPtr& rhs2) :
      op(op), rhs1(rhs1), rhs2(rhs2)
    { }
    Opcode getOpcode() const { return op; }
    const MachineOperandPtr& getRhs1() const { return rhs1; }
    const MachineOperandPtr& getRhs2() const { return rhs2; }
    std::ostream& printTo(std::ostream& stream) const override;
  private:
    Opcode op;
    MachineOperandPtr rhs1;
    MachineOperandPtr rhs2;
  };

  MachineInsnPtr buildMovInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildMovSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildMovZblInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildMovEqualInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
	MachineInsnPtr buildMovDwInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildSubInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildSubSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildAddInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildAddSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildIMulInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildIDivInsn(const MachineOperandPtr& src);
  MachineInsnPtr buildMulSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildDivSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildCmpInsn(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs);
  MachineInsnPtr buildUComIssInsn(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs);
  MachineInsnPtr buildXorInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildXorSsInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildNegInsn(const MachineOperandPtr& dst);
  MachineInsnPtr buildCallInsn(const MachineOperandPtr& callee);
  MachineInsnPtr buildJmpInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildJmpEqualInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpNotEqualInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpLessEqualInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpLessInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpGreaterEqualInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpGreaterInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpAboveInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpNotAboveInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpBelowInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildJmpNotBelowInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetEqualInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetNotEqualInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetLessEqualInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetLessInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetGreaterEqualInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetGreaterInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetParityInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetNotParityInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetAboveInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetNotAboveInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetBelowInsn(const MachineOperandPtr& target);
  MachineInsnPtr buildSetNotBelowInsn(const MachineOperandPtr& target);
	MachineInsnPtr buildLabelInsn(const MachineOperandPtr& label);
	MachineInsnPtr buildLeaInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
	MachineInsnPtr buildSalInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
	MachineInsnPtr buildSarInsn(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  MachineInsnPtr buildRetInsn();

  // TODO bei TemplateInsn .. setInputOperands / setOutputOperands / setClobbers
  class TemplateInsn : public Printable {
  public:
    TemplateInsn(const MachineInsnList& insns) : insns(insns) {}
    MachineInsnList& getInsns() { return insns; }
    const MachineInsnList& getInsns() const { return insns; }
    std::ostream& printTo(std::ostream& stream) const override;

    static void append(const TemplateInsnPtr& templateInsn, const MachineInsnPtr& insn);
    static void append(const TemplateInsnPtr& templateInsn, const TemplateInsnPtr& insn);
    static void prepend(const TemplateInsnPtr& templateInsn, const MachineInsnPtr& insn);
    static void prepend(const TemplateInsnPtr& templateInsn, const TemplateInsnPtr& insn);
  private:
    MachineInsnList insns;
  };

  TemplateInsnPtr buildFrameEntryTemplate(const memory::StackFramePtr& frame);
  TemplateInsnPtr buildFrameLeaveTemplate(const memory::StackFramePtr& frame);
  // in order to abstract the difference between single-precision xmm0 & gpr away from the user
  TemplateInsnPtr buildPushTemplate(const MachineOperandPtr& val);
  TemplateInsnPtr buildPopTemplate(const MachineOperandPtr& val);
  TemplateInsnPtr buildMovTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  TemplateInsnPtr buildAddTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  TemplateInsnPtr buildSubTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  TemplateInsnPtr buildCmpTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs);
  TemplateInsnPtr buildMulTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  TemplateInsnPtr buildDivTemplate(const MachineOperandPtr& src, const MachineOperandPtr& dst);
  TemplateInsnPtr buildEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst);
  TemplateInsnPtr buildNotEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst);
  TemplateInsnPtr buildLessEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst);
  TemplateInsnPtr buildLessTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst);
  TemplateInsnPtr buildGreaterEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst);
  TemplateInsnPtr buildGreaterTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& dst);
	TemplateInsnPtr buildJmpEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target);
  TemplateInsnPtr buildJmpNotEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target);
  TemplateInsnPtr buildJmpLessEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target);
  TemplateInsnPtr buildJmpLessTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target);
  TemplateInsnPtr buildJmpGreaterEqualTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target);
  TemplateInsnPtr buildJmpGreaterTemplate(const MachineOperandPtr& lhs, const MachineOperandPtr& rhs, const MachineOperandPtr& target);
  TemplateInsnPtr buildNegTemplate(const MachineOperandPtr& dst);
  TemplateInsnPtr buildNotTemplate(const MachineOperandPtr& dst);
}
}
