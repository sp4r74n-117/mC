#include "core.h"
#include "core/analysis/analysis.h"
#include "core/arithmetic/arithmetic.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis-callgraph.h"
#include "core/analysis/analysis-controlflow.h"
#include <algorithm>
#include <fstream>

namespace core {
	namespace detail {
		template<typename T>
		bool replaceIf(Ptr<T>& member, const NodePtr& target, const NodePtr& replacement) {
			if (!member) return false;
			if (*member != *target) return false;

			member = dyn_cast<T>(replacement);
			assert(member && "replacement has wrong type!");
			return true;
		}
	}

	std::ostream& operator <<(std::ostream& stream, const AssignInsn::OpType& op) {
		switch (op) {
		case AssignInsn::ADD:	stream << "+"; break;
		case AssignInsn::SUB:	stream << "-"; break;
		case AssignInsn::MUL:	stream << "*"; break;
		case AssignInsn::DIV:	stream << "/"; break;
		case AssignInsn::EQ:	stream << "=="; break;
		case AssignInsn::NE:	stream << "!="; break;
		case AssignInsn::LE:	stream << "<="; break;
		case AssignInsn::GE:	stream << ">="; break;
		case AssignInsn::LT:	stream << "<"; break;
		case AssignInsn::GT:	stream << ">"; break;
		case AssignInsn::NOT:	stream << "!"; break;
		default:
				assert(false && "Unsupported operand");
				break;
		}
		return stream;
	}

	std::string NodeManager::getUniqueTemporaryName() {
		return "$" + std::to_string(tmpNr++);
	}

	std::string NodeManager::getUniqueLabelName() {
		return "L" + std::to_string(lblNr++);
	}

	TypePtr NodeManager::buildBasicType(Type::TypeId typeId) {
		assert(typeId != Type::TI_Function && "invalid typeid passed");
		auto ptr = std::make_shared<Type>(typeId);
		return types.add(ptr);
	}

	ArrayTypePtr NodeManager::buildArrayType(const TypePtr& elementType, const unsigned numOfDims) {
		auto ptr = std::make_shared<ArrayType>(elementType, numOfDims);
		return types.add(ptr);
	}

	FunctionTypePtr NodeManager::buildFunctionType(const TypePtr& returnType, const TypeList& parameterTypes) {
		auto ptr = std::make_shared<FunctionType>(returnType, parameterTypes);
		return types.add(ptr);
	}

	AllocaInsnPtr NodeManager::buildAlloca(const VariablePtr& variable, const ValuePtr& size) {
		ValueList dimensions;
		auto alloca = std::make_shared<AllocaInsn>(variable, size, dimensions);
		variable->setParent(alloca);
		return alloca;
	}

	AllocaInsnPtr NodeManager::buildAlloca(const VariablePtr& variable, const ValuePtr& size, const ValueList& dimensions) {
		auto alloca = std::make_shared<AllocaInsn>(variable, size, dimensions);
		variable->setParent(alloca);
		return alloca;
	}

	LoadInsnPtr NodeManager::buildLoad(const VariablePtr& source, const VariablePtr& target) {
		return std::make_shared<LoadInsn>(source, target);
	}

	StoreInsnPtr NodeManager::buildStore(const ValuePtr& source, const VariablePtr& target) {
		return std::make_shared<StoreInsn>(source, target);
	}

	PushSpInsnPtr NodeManager::buildPushSp(const VariablePtr& rhs) {
		return std::make_shared<PushSpInsn>(rhs);
	}

	PopSpInsnPtr NodeManager::buildPopSp(const VariablePtr& rhs) {
		return std::make_shared<PopSpInsn>(rhs);
	}

	VariablePtr NodeManager::buildVariable(const TypePtr& type, const std::string& name) {
		auto ptr = std::make_shared<Variable>(Value::VC_Memory, Value::VT_Memory, type, name);
		return values.add(ptr);
	}

	VariablePtr NodeManager::buildOffset(const TypePtr& type) {
		auto ptr = std::make_shared<Variable>(Value::VC_Temporary, Value::VT_Memory, type, getUniqueTemporaryName());
		return values.add(ptr);
	}

	VariablePtr NodeManager::buildTemporary(const TypePtr& type) {
		auto ptr = std::make_shared<Variable>(Value::VC_Temporary, Value::VT_Temporary, type, getUniqueTemporaryName());
		return values.add(ptr);
	}

	ValuePtr NodeManager::buildIntConstant(int value) {
		auto ptr = std::make_shared<IntConstant>(value);
		return values.add(ptr);
	}

	ValuePtr NodeManager::buildFloatConstant(float value) {
		auto ptr = std::make_shared<FloatConstant>(value);
		return values.add(ptr);
	}

	AssignInsnPtr NodeManager::buildAssign(const VariablePtr& lhs, const ValuePtr& rhs1) {
		return std::make_shared<AssignInsn>(lhs, rhs1);
	}

	AssignInsnPtr NodeManager::buildAssign(AssignInsn::OpType op, const VariablePtr& lhs, const ValuePtr& rhs1) {
		return std::make_shared<AssignInsn>(op, lhs, rhs1);
	}

	AssignInsnPtr NodeManager::buildAssign(AssignInsn::OpType op, const VariablePtr& lhs, const ValuePtr& rhs1, const ValuePtr& rhs2) {
		return std::make_shared<AssignInsn>(op, lhs, rhs1, rhs2);
	}

	LabelInsnPtr NodeManager::buildLabel() {
		return std::make_shared<LabelInsn>(getUniqueLabelName());
	}

	GotoInsnPtr NodeManager::buildGoto(const LabelInsnPtr& target) {
		return std::make_shared<GotoInsn>(target);
	}

	FalseJumpInsnPtr NodeManager::buildFalseJump(const ValuePtr& cond, const LabelInsnPtr& target) {
		return std::make_shared<FalseJumpInsn>(cond, target);
	}

	PhiInsnPtr NodeManager::buildPhi(const VariablePtr& lhs, const VariableList& rhs) {
		return std::make_shared<PhiInsn>(lhs, rhs);
	}

	FunctionPtr NodeManager::buildFunction(const std::string& name, const FunctionTypePtr& type, const VariableList& parameters) {
		// label checker will assure uniqueness -- however that should not fire if fe works as expected
		auto label = std::make_shared<LabelInsn>(name);
		return std::make_shared<Function>(label, type, parameters);
	}

	ReturnInsnPtr NodeManager::buildReturn() {
		return std::make_shared<ReturnInsn>(nullptr);
	}

	ReturnInsnPtr NodeManager::buildReturn(const ValuePtr& value) {
		return std::make_shared<ReturnInsn>(value);
	}

	PushInsnPtr NodeManager::buildPush(const ValuePtr& value) {
		return std::make_shared<PushInsn>(value);
	}

	PopInsnPtr NodeManager::buildPop(const ValuePtr& value) {
		return std::make_shared<PopInsn>(value);
	}

	PopInsnPtr NodeManager::buildPop(size_t numOfBytes) {
		return buildPop(buildIntConstant(numOfBytes));
	}

	CallInsnPtr NodeManager::buildCall(const FunctionPtr& callee) {
			return std::make_shared<CallInsn>(callee);
	}

	CallInsnPtr NodeManager::buildCall(const FunctionPtr& callee, const VariablePtr& result) {
		return std::make_shared<CallInsn>(callee, result);
	}

	std::ostream& NodeManager::printTo(std::ostream& stream) const {
		return getProgram()->printTo(stream);
	}

	bool BasicBlock::isValid() const {
		// in case we have no label, fail-fast
		if (!getLabel()) return false;
		// now check all instructions for validity
		if (getInsns().empty()) return true;

		auto last = getInsns().end() - 1;
		auto it = std::find_if(getInsns().begin(), last,
			[&](const InsnPtr& insn) { return insn->getInsnCategory() == Insn::IC_Termination; });
		// in this case we have a termination insn within the insn stream!
		if (it != last) return false;
		return (*last)->getInsnType() != Insn::IT_Label;
	}

	bool Node::operator!=(const Node& other) const {
		return !(*this == other);
	}

	bool Type::operator==(const Node& other) const {
		if (typeid(Type) != typeid(other)) return false;
		return id == static_cast<const Type&>(other).id;
	}

	std::ostream& Type::printTo(std::ostream& stream) const {
		switch (id) {
		case Type::TI_Int: 	stream << "int"; break;
		case Type::TI_Float: 	stream << "float"; break;
		case Type::TI_Void:	stream << "void"; break;
		case Type::TI_Function:	stream << "function"; break;
		case Type::TI_Array: stream << "array"; break;
		}
		return stream;
	}

	bool ArrayType::operator ==(const Node& other) const {
		if (typeid(other) != typeid(ArrayType)) return false;
		return *elementType == *static_cast<const ArrayType&>(other).elementType &&
					 numOfDims == static_cast<const ArrayType&>(other).numOfDims;
	}

	std::ostream& ArrayType::printTo(std::ostream& stream) const {
		elementType->printTo(stream);
		return stream << std::to_string(numOfDims);
	}

	bool FunctionType::operator==(const Node& other) const {
		if (typeid(other) != typeid(FunctionType)) return false;
		return *returnType == *static_cast<const FunctionType&>(other).returnType &&
					 parameterTypes == static_cast<const FunctionType&>(other).parameterTypes;
	}

	std::ostream& FunctionType::printTo(std::ostream& stream) const {
		returnType->printTo(stream);
		stream << "(";
		bool first = true;
		for (const auto& type : parameterTypes) {
			if (first) first = false;
			else		   stream << ",";
			type->printTo(stream);
		}
		return stream << ")";
	}

	void Variable::setSSAIndex(unsigned index) {
		ssaIndex = index;
	}

	std::string Variable::getName() const {
		if (hasSSAIndex())
			return name + "." + std::to_string(ssaIndex);
		else
			return name;
	}

	bool Variable::operator==(const Node& other) const {
		if (typeid(Variable) != typeid(other)) return false;
		return name == static_cast<const Variable&>(other).name;
	}

	bool Variable::operator <(const Value& rhs) const {
		if (typeid(Variable) != typeid(rhs)) return false;
		return name < static_cast<const Variable&>(rhs).name;
	}

	std::ostream& Variable::printTo(std::ostream& stream) const {
		return stream << getName();
	}

	bool IntConstant::operator==(const Node& other) const {
		if (typeid(IntConstant) != typeid(other)) return false;
		return value == static_cast<const IntConstant&>(other).value;
	}

	bool IntConstant::operator <(const Value& rhs) const {
		if (typeid(IntConstant) != typeid(rhs)) return false;
		return value < static_cast<const IntConstant&>(rhs).value;
	}

	std::ostream& IntConstant::printTo(std::ostream& stream) const {
		return stream << value;
	}

	bool FloatConstant::operator==(const Node& other) const {
		if (typeid(FloatConstant) != typeid(other)) return false;
		return value == static_cast<const FloatConstant&>(other).value;
	}

	bool FloatConstant::operator <(const Value& rhs) const {
		if (typeid(FloatConstant) != typeid(rhs)) return false;
		return value < static_cast<const FloatConstant&>(rhs).value;
	}

	std::ostream& FloatConstant::printTo(std::ostream& stream) const {
		return stream << value;
	}

	bool LabelInsn::operator <(const LabelInsn& other) const {
		return name < other.name;
	}

	bool LabelInsn::operator==(const Node& other) const {
		if (typeid(LabelInsn) != typeid(other)) return false;
		return name == static_cast<const LabelInsn&>(other).name;
	}

	std::ostream& LabelInsn::printTo(std::ostream& stream) const {
		return stream << "label " << name;
	}

	void GotoInsn::replaceNode(const NodePtr& target, const NodePtr& replacement) {
		detail::replaceIf(this->target, target, replacement);
	}

	bool GotoInsn::operator==(const Node& other) const {
		if (typeid(GotoInsn) != typeid(other)) return false;
		return *target == *static_cast<const GotoInsn&>(other).target;
	}

	std::ostream& GotoInsn::printTo(std::ostream& stream) const {
		return stream << "goto " << getTarget()->getName();
	}

	void FalseJumpInsn::replaceNode(const NodePtr& target, const NodePtr& replacement) {
		if (detail::replaceIf(cond, target, replacement)) return;
		if (detail::replaceIf(this->target, target, replacement)) return;
	}

	bool FalseJumpInsn::operator==(const Node& other) const {
		if (typeid(FalseJumpInsn) != typeid(other)) return false;
		if (*cond != *static_cast<const FalseJumpInsn&>(other).cond) return false;
		if (*target != *static_cast<const FalseJumpInsn&>(other).target) return false;
		return true;
	}

	std::ostream& FalseJumpInsn::printTo(std::ostream& stream) const {
		stream << "fjmp ";
		getCond()->printTo(stream);
		return stream << " " << getTarget()->getName();
	}

	bool AssignInsn::operator==(const Node& other) const {
		if (typeid(AssignInsn) != typeid(other)) return false;
		if (op != static_cast<const AssignInsn&>(other).op) return false;
		if (*lhs != *static_cast<const AssignInsn&>(other).lhs) return false;
		if (*rhs1 != *static_cast<const AssignInsn&>(other).rhs1) return false;
		// in case we have rhs2 compare that too
		if (rhs2 && *rhs2 != *static_cast<const AssignInsn&>(other).rhs2) return false;
		return true;
	}

	std::ostream& AssignInsn::printTo(std::ostream& stream) const {
		getLhs()->printTo(stream);
		stream << " = ";
		if (isAssign()) {
			getRhs1()->printTo(stream);
		} else if (isUnary()) {
			stream << op;
			getRhs1()->printTo(stream);
		} else {
			getRhs1()->printTo(stream);
			stream << op;
			getRhs2()->printTo(stream);
		}
		return stream;
	}

	bool AssignInsn::isUnaryOp(AssignInsn::OpType op) {
		switch (op) {
		case AssignInsn::NOT:
		case AssignInsn::SUB:
				return true;
		default:
				return false;
		}
	}

	bool AssignInsn::isBinaryOp(AssignInsn::OpType op) {
		switch (op) {
		case AssignInsn::ADD:
		case AssignInsn::SUB:
		case AssignInsn::MUL:
		case AssignInsn::DIV:
				return true;
		default:
				return isLogicalBinaryOp(op);
		}
	}

	bool AssignInsn::isLogicalBinaryOp(AssignInsn::OpType op) {
		switch (op) {
		case AssignInsn::EQ:
		case AssignInsn::NE:
		case AssignInsn::LE:
		case AssignInsn::GE:
		case AssignInsn::LT:
		case AssignInsn::GT:
				return true;
		default:
				return false;
		}
	}

	void AssignInsn::replaceNode(const NodePtr& target, const NodePtr& replacement) {
		if (detail::replaceIf(rhs1, target, replacement)) return;
		if (detail::replaceIf(rhs2, target, replacement)) return;
	}

	bool PhiInsn::operator==(const Node& other) const {
		if (typeid(PhiInsn) != typeid(other)) return false;
		if (*lhs != *static_cast<const PhiInsn&>(other).lhs) return false;
		if (rhs != static_cast<const PhiInsn&>(other).rhs) return false;
		return true;
	}

	std::ostream& PhiInsn::printTo(std::ostream& stream) const {
		getLhs()->printTo(stream);
		stream << " = phi(";
		bool first = true;
		for (const auto& cur : getRhs()) {
			if (first) { first = false; }
			else	   stream << ",";
			cur->printTo(stream);
		}
		stream << ")";
		return stream;
	}

	void ReturnInsn::replaceNode(const NodePtr& target, const NodePtr& replacement) {
		detail::replaceIf(rhs, target, replacement);
	}

	bool ReturnInsn::operator==(const Node& other) const {
		if (typeid(ReturnInsn) != typeid(other)) return false;

		const auto& o = static_cast<const ReturnInsn&>(other);
		if (rhs && o.rhs && *rhs == *o.rhs) return true;
		if (!rhs && !o.rhs) return true;
		return false;
	}

	std::ostream& ReturnInsn::printTo(std::ostream& stream) const {
		stream << "ret";
		if (rhs) rhs->printTo(stream << " ");
		return stream;
	}

	void PushInsn::replaceNode(const NodePtr& target, const NodePtr& replacement) {
		detail::replaceIf(rhs, target, replacement);
	}

	bool PushInsn::operator==(const Node& other) const {
		if (typeid(PushInsn) != typeid(other)) return false;
		if (*rhs != *static_cast<const PushInsn&>(other).rhs) return false;
		return true;
	}

	std::ostream& PushInsn::printTo(std::ostream& stream) const {
		stream << "push ";
		return rhs->printTo(stream);
	}

	unsigned PopInsn::getNumOfBytes() const {
		if (analysis::isIntConstant(getRhs())) {
			return arithmetic::getValue<unsigned>(getRhs());
		} else {
			return 4;
		}
	}

	void PopInsn::replaceNode(const NodePtr& target, const NodePtr& replacement) {
		detail::replaceIf(rhs, target, replacement);
	}

	bool PopInsn::operator==(const Node& other) const {
		if (typeid(PopInsn) != typeid(other)) return false;
		if (*rhs != *static_cast<const PopInsn&>(other).rhs) return false;
		return true;
	}

	std::ostream& PopInsn::printTo(std::ostream& stream) const {
		stream << "pop ";
		return rhs->printTo(stream);
	}

	CallInsn::CallInsn(const FunctionPtr& callee, const VariablePtr& result) :
		Insn(IC_Call, IT_Call), callee(callee), result(result) {
		assert(callee && "callee must not be null");
		// we cannot check via isCallable as we do not have stack information at this point!
		assert(analysis::types::hasReturn(callee->getType()) && "callee must return a value");
	}

	bool CallInsn::operator==(const Node& other) const {
		if (typeid(CallInsn) != typeid(other)) return false;

		const auto& o = static_cast<const CallInsn&>(other);
		if (*callee != *o.callee) return false;
		if (result && o.result && *result == *o.result) return true;
		if (!result && !o.result) return true;
		return false;
	}

	std::ostream& CallInsn::printTo(std::ostream& stream) const {
		stream << "call " << callee->getName();
		if (result) return result->printTo(stream << ",");
		else 			  return stream;
	}

	AllocaInsn::AllocaInsn(const VariablePtr& variable, const ValuePtr& size, const ValueList& dimensions) :
		Insn(IC_Stack, IT_Alloca), size(size), variable(variable), dimensions(dimensions) {
		assert(variable && "variable must not be null");
		assert(variable->getValueType() == Value::VT_Memory && "variable must not be a temporary");
		assert(size && "size must not be null");
		assert(size->getType()->isInt() && "size must be of type int");

		if (analysis::types::isArray(variable->getType())) {
			auto type = cast<ArrayType>(variable->getType());
			assert(type->getNumOfDimensions() == dimensions.size() && "dimension information mismatch with type");
		}
	}

	bool AllocaInsn::operator==(const Node& other) const {
		if (typeid(AllocaInsn) != typeid(other)) return false;

		const auto& o = static_cast<const AllocaInsn&>(other);
		if (*o.variable != *variable) return false;
		if (*o.size != *size) return false;
		return true;
	}

	std::ostream& AllocaInsn::printTo(std::ostream& stream) const {
		variable->printTo(stream << "alloca ");
		size->printTo(stream << ",");
		return variable->getType()->printTo(stream << " ");
	}

	void LoadInsn::replaceNode(const NodePtr &target, const NodePtr &replacement) {
		if (detail::replaceIf(source, target, replacement)) return;
		if (detail::replaceIf(this->target, target, replacement)) return;
	}

	bool LoadInsn::operator==(const Node& other) const {
		if (typeid(LoadInsn) != typeid(other)) return false;

		const auto& o = static_cast<const LoadInsn&>(other);
		if (*o.source != *source) return false;
		if (*o.target != *target) return false;
		return true;
	}

	std::ostream& LoadInsn::printTo(std::ostream& stream) const {
		source->printTo(stream << "load ");
		return target->printTo(stream << ",");
	}

	void StoreInsn::replaceNode(const NodePtr &target, const NodePtr &replacement) {
		if (detail::replaceIf(source, target, replacement)) return;
		if (detail::replaceIf(this->target, target, replacement)) return;
	}

	bool StoreInsn::operator==(const Node& other) const {
		if (typeid(StoreInsn) != typeid(other)) return false;

		const auto& o = static_cast<const StoreInsn&>(other);
		if (*o.source != *source) return false;
		if (*o.target != *target) return false;
		return true;
	}

	std::ostream& StoreInsn::printTo(std::ostream& stream) const {
		source->printTo(stream << "store ");
		return target->printTo(stream << ",");
	}

	bool PushSpInsn::operator==(const Node& other) const {
		if (typeid(PushSpInsn) != typeid(other)) return false;
		if (*rhs != *static_cast<const PushSpInsn&>(other).rhs) return false;
		return true;
	}

	std::ostream& PushSpInsn::printTo(std::ostream& stream) const {
		stream << "pushsp ";
		return rhs->printTo(stream);
	}

	bool PopSpInsn::operator==(const Node& other) const {
		if (typeid(PopSpInsn) != typeid(other)) return false;
		if (*rhs != *static_cast<const PopSpInsn&>(other).rhs) return false;
		return true;
	}

	std::ostream& PopSpInsn::printTo(std::ostream& stream) const {
		stream << "popsp ";
		return rhs->printTo(stream);
	}

	bool BasicBlock::operator <(const BasicBlock& other) const {
		return *lbl < *other.lbl;
	}

	bool BasicBlock::operator ==(const BasicBlock& other) const {
		return *lbl == *other.lbl;
	}

	std::ostream& BasicBlock::printTo(std::ostream& stream) const {
		stream << lbl->getName() << " {" << std::endl;
		for (const auto& insn : insns) {
			insn->printTo(stream);
			stream << std::endl;
		}
		return stream << "}";
	}

	bool Function::operator==(const Node& other) const {
		if (typeid(Function) != typeid(other)) return false;
		return *label == *static_cast<const Function&>(other).label;
	}

	std::ostream& Function::printTo(std::ostream& stream) const {
		return analysis::controlflow::ControFlowPrinter(*this).printTo(stream);
	}

	bool Program::operator==(const Node& other) const {
		if (typeid(Program) != typeid(other)) return false;
		return functions == static_cast<const Program&>(other).functions;
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
	}

	bool dumpTo(const ProgramPtr& program, const std::string& dir) {
		// first of all, we want to dump the callgraph
		auto callgraph = analysis::callgraph::getCallGraph(program);
		if (!dumpTo(dir + "/callgraph.dot", toString(callgraph))) return false;
		// finally, generate a file for each function
		for (const auto& fun : program->getFunctions()) {
			if (!dumpTo(dir + "/" + analysis::callgraph::getFunctionName(fun) + ".dot", toString(*fun))) return false;
		}
		return true;
	}
}
