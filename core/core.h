#pragma once
#include "utils/utils.h"
#include <map>
#include <set>
#include <cassert>
#include <typeinfo>
#include <climits>
#include <functional>

namespace core {
	class Location;
	typedef Ptr<Location> LocationPtr;
	class Node;
	typedef Ptr<Node> NodePtr;
	class Type;
	typedef Ptr<Type> TypePtr;
	class ArrayType;
	typedef Ptr<ArrayType> ArrayTypePtr;
	class FunctionType;
	typedef Ptr<FunctionType> FunctionTypePtr;
	class Value;
	typedef Ptr<Value> ValuePtr;
	class IntConstant;
	typedef Ptr<IntConstant> IntConstantPtr;
	class FloatConstant;
	typedef Ptr<FloatConstant> FloatConstantPtr;
	class Variable;
	typedef Ptr<Variable> VariablePtr;
	class Insn;
	typedef Ptr<Insn> InsnPtr;
	class AssignInsn;
	typedef Ptr<AssignInsn> AssignInsnPtr;
	class LabelInsn;
	typedef Ptr<LabelInsn> LabelInsnPtr;
	class GotoInsn;
	typedef Ptr<GotoInsn> GotoInsnPtr;
	class FalseJumpInsn;
	typedef Ptr<FalseJumpInsn> FalseJumpInsnPtr;
	class PhiInsn;
	typedef Ptr<PhiInsn> PhiInsnPtr;
	class ReturnInsn;
	typedef Ptr<ReturnInsn> ReturnInsnPtr;
	class PushInsn;
	typedef Ptr<PushInsn> PushInsnPtr;
	class PopInsn;
	typedef Ptr<PopInsn> PopInsnPtr;
	class CallInsn;
	typedef Ptr<CallInsn> CallInsnPtr;
	class Program;
	typedef Ptr<Program> ProgramPtr;
	class Function;
	typedef Ptr<Function> FunctionPtr;
	class AllocaInsn;
	typedef Ptr<AllocaInsn> AllocaInsnPtr;
	class LoadInsn;
	typedef Ptr<LoadInsn> LoadInsnPtr;
	class StoreInsn;
	typedef Ptr<StoreInsn> StoreInsnPtr;
	class PushSpInsn;
	typedef Ptr<PushSpInsn> PushSpInsnPtr;
	class PopSpInsn;
	typedef Ptr<PopSpInsn> PopSpInsnPtr;

	class BasicBlock;
	typedef typename DirectedGraph<BasicBlock>::vertex_type BasicBlockPtr;
	typedef typename DirectedGraph<BasicBlock>::edge_type EdgePtr;
	typedef typename DirectedGraph<BasicBlock>::vertex_list_type BasicBlockList;
	typedef typename DirectedGraph<BasicBlock>::edge_list_type EdgeList;

	typedef PtrList<Node> NodeList;
	typedef PtrList<Insn> InsnList;
	typedef PtrList<Value> ValueList;
	typedef PtrList<Variable> VariableList;
	typedef PtrList<Type> TypeList;
	typedef PtrList<Function> FunctionList;
	typedef PtrSet<Variable> VariableSet;

	class Location : public Printable {
		unsigned row;
	public:
		Location(unsigned row) : row(row) {}
		unsigned getRow() const { return row; }
		bool operator <(const Location& other) const { return row < other.row; }
		bool operator==(const Location& other) const { return row == other.row; }
		bool operator!=(const Location& other) const { return !(*this == other); }
		std::ostream& printTo(std::ostream& stream) const override { return stream << "line: " << row; }
	};

	class Node : public Printable {
	public:
		enum NodeCategory { NC_Value, NC_Insn, NC_Type, NC_Function, NC_Program };
		NodeCategory getNodeCategory() { return category; }
		bool hasLocation() const { return location != nullptr; }
		const LocationPtr& getLocation() const { return location; }
		void setLocation(const LocationPtr& location) { this->location = location; }
		virtual void replaceNode(const NodePtr& target, const NodePtr& replacement) { }
		virtual bool operator==(const Node& other) const = 0;
		bool operator!=(const Node& other) const;
	protected:
		Node(NodeCategory category) :
			category(category)
		{ }
	private:
		NodeCategory category;
		LocationPtr location;
	};

	class Type : public Node {
	public:
		enum TypeId {
			TI_Int,
			TI_Float,
			TI_Void,
			TI_Array,
			TI_Function
		};
		Type(const TypeId& id) : Node(NC_Type), id(id) { }
		bool isInt() const { return id == TI_Int; }
		bool isFloat() const { return id == TI_Float; }
		bool isVoid() const { return id == TI_Void; }
		bool isFunction() const { return id == TI_Function; }
		bool isArray() const { return id == TI_Array; }
		TypeId getTypeId() const { return id; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	private:
		TypeId id;
	};

	class ArrayType : public Type {
	public:
		ArrayType(const TypePtr& elementType, const unsigned numOfDims) :
			Type(TI_Array), elementType(elementType), numOfDims(numOfDims) {
			assert(elementType && "elementType must not be null");
			assert(numOfDims && "numOfDims must be greater than zero");
		}
		const TypePtr& getElementType() const { return elementType; }
		unsigned getNumOfDimensions() const { return numOfDims; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	private:
		TypePtr elementType;
		unsigned numOfDims;
	};

	class FunctionType : public Type {
	public:
		FunctionType(const TypePtr& returnType, const TypeList& parameterTypes) :
			Type(TI_Function), returnType(returnType), parameterTypes(parameterTypes) {}
		const TypePtr& getReturnType() const { return returnType; }
		const TypeList& getParameterTypes() const { return parameterTypes; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	private:
		TypePtr returnType;
		TypeList parameterTypes;
	};

	class Value : public Node {
	public:
		enum ValueCategory { VC_Memory, VC_Temporary, VC_Constant };
		enum ValueType { VT_Memory, VT_Temporary, VT_IntConstant, VT_FloatConstant, VT_StringConstant };
		ValueCategory getValueCategory() const { return category; }
		ValueType getValueType() const { return valueType; }
		const TypePtr& getType() const { return type; }
		virtual bool operator <(const Value& rhs) const = 0;
	protected:
		Value(ValueCategory category, ValueType valueType, const TypePtr& type) :
			Node(NC_Value), category(category), valueType(valueType), type(type) { }
	private:
		ValueCategory category;
		ValueType valueType;
		TypePtr type;
	};

	class Variable : public Value {
		std::string name;
		unsigned ssaIndex;
		AllocaInsnPtr parent;
	public:
		Variable(ValueCategory valueCategory, ValueType valueType, const TypePtr& type, const std::string& name) :
			Value(valueCategory, valueType, type), name(name), ssaIndex(UINT_MAX)
		{ }
		bool hasSSAIndex() const { return ssaIndex != UINT_MAX; }
		unsigned getSSAIndex() const { return ssaIndex; }
		void setSSAIndex(unsigned index);
		std::string getName() const;
		bool hasParent() const { return parent != nullptr; }
		const AllocaInsnPtr& getParent() const { assert(hasParent() && "var has no parent"); return parent; }
		void setParent(const AllocaInsnPtr& parent) { this->parent = parent; }
		bool operator==(const Node& other) const override;
		bool operator <(const Value& rhs) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class IntConstant : public Value {
		int value;
	public:
		IntConstant(int value) :
			Value(VC_Constant, VT_IntConstant, std::make_shared<Type>(Type::TI_Int)), value(value)
		{ }
		int getValue() const { return value; }
		bool operator==(const Node& other) const override;
		bool operator <(const Value& rhs) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class FloatConstant : public Value {
		float value;
	public:
		FloatConstant(float value) :
			Value(VC_Constant, VT_FloatConstant, std::make_shared<Type>(Type::TI_Float)), value(value)
		{ }
		float getValue() const { return value; }
		bool operator==(const Node& other) const override;
		bool operator <(const Value& rhs) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class Insn : public Node {
	public:
		enum InsnCategory { IC_Termination, IC_Assign, IC_SSA, IC_Stack, IC_Call };
		enum InsnType { IT_Assign, IT_Goto, IT_FalseJump, IT_Label, IT_Phi, IT_Return,
			IT_Push, IT_Pop, IT_Call, IT_Alloca, IT_Load, IT_Store, IT_PushSp, IT_PopSp };
		InsnCategory getInsnCategory() const { return category; }
		InsnType getInsnType() const { return insnType; }
		bool hasParent() const { return parent != nullptr; }
		const BasicBlockPtr& getParent() const { assert(hasParent() && "insn has no parent"); return parent; }
		void setParent(const BasicBlockPtr& parent) { this->parent = parent; }
	protected:
		Insn(InsnCategory category, InsnType insnType) :
			Node(NC_Insn), category(category), insnType(insnType)
		{ }
	private:
		InsnCategory category;
		InsnType insnType;
		BasicBlockPtr parent;
	};

	class AssignInsn : public Insn {
	public:
		enum OpType { ADD, SUB, MUL, DIV, EQ, NE, LE, GE, LT, GT, NOT, NONE };
		AssignInsn(const VariablePtr& lhs, const ValuePtr& rhs1) :
			Insn(IC_Assign, IT_Assign), op(NONE), lhs(lhs), rhs1(rhs1) {
			assert(lhs && rhs1 && "lhs/rhs1 must not be null");
		}
		AssignInsn(OpType op, const VariablePtr& lhs, const ValuePtr& rhs1) :
			Insn(IC_Assign, IT_Assign), op(op), lhs(lhs), rhs1(rhs1) {
			assert(lhs && rhs1 && "lhs/rhs1 must not be null");
			assert((op == SUB || op == NOT) && "unary operation must be SUB or NOT");
		}
		AssignInsn(OpType op, const VariablePtr& lhs, const ValuePtr& rhs1, const ValuePtr& rhs2) :
			Insn(IC_Assign, IT_Assign), op(op), lhs(lhs), rhs1(rhs1), rhs2(rhs2) {
			assert(lhs && rhs1 && rhs2 && "lhs/rhs1/rhs2 must not be null");
			assert(op != NOT && "a binary operation must not be a NOT");
		}
		bool isUnary() const { return (op == SUB || op == NOT) && !rhs2; }
		bool isAssign() const { return op == NONE && !rhs2; }
		bool isBinary() const { return !isUnary() && !isAssign(); }
		OpType getOp() const { return op; }
		const VariablePtr& getLhs() const { return lhs; }
		const ValuePtr& getRhs1() const { return rhs1; }
		const ValuePtr& getRhs2() const { return rhs2; }
		void setOp(OpType op) { this->op = op; }
		void setLhs(const VariablePtr& lhs) { this->lhs = lhs; }
		void setRhs1(const ValuePtr& rhs1) { this->rhs1 = rhs1; }
		void setRhs2(const ValuePtr& rhs2) { this->rhs2 = rhs2; }
		bool operator==(const Node& other) const override;
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		std::ostream& printTo(std::ostream& stream) const override;

		static bool isUnaryOp(AssignInsn::OpType op);
		static bool isBinaryOp(AssignInsn::OpType op);
		static bool isLogicalBinaryOp(AssignInsn::OpType op);
	private:
		OpType op;
		VariablePtr lhs;
		ValuePtr rhs1;
		ValuePtr rhs2;
	};

	class LabelInsn : public Insn {
		std::string name;
	public:
		LabelInsn(const std::string& name) :
			Insn(IC_Termination, IT_Label), name(name)
		{ }
		const std::string& getName() const { return name; }
		bool operator <(const LabelInsn& other) const;
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class GotoInsn : public Insn {
		LabelInsnPtr target;
	public:
		GotoInsn(const LabelInsnPtr& target) :
			Insn(IC_Termination, IT_Goto), target(target) {
			assert(target && "goto target must not be null");
		}
		const LabelInsnPtr& getTarget() const { return target; }
		bool operator==(const Node& other) const override;
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class FalseJumpInsn : public Insn {
		ValuePtr cond;
		LabelInsnPtr target;
	public:
		FalseJumpInsn(const ValuePtr& cond, const LabelInsnPtr& target) :
			Insn(IC_Termination, IT_FalseJump), cond(cond), target(target) {
			assert(cond && target && "fjmp cond and target must not be null");
		}
		const ValuePtr& getCond() const { return cond; }
		const LabelInsnPtr& getTarget() const { return target; }
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class PhiInsn : public Insn {
		VariablePtr lhs;
		VariableList rhs;
	public:
		PhiInsn(const VariablePtr& lhs, const VariableList& rhs) :
			Insn(IC_SSA, IT_Phi), lhs(lhs), rhs(rhs) {
			assert(lhs && rhs.size() && "lhs or rhs of phi cannot be null");
		}
		void setLhs(const VariablePtr& lhs) { this->lhs = lhs; }
		const VariablePtr& getLhs() const { return lhs; }
		VariableList& getRhs() { return rhs; }
		const VariableList& getRhs() const { return rhs; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class ReturnInsn : public Insn {
		ValuePtr rhs;
	public:
		ReturnInsn(const ValuePtr& rhs) :
			Insn(IC_Termination, IT_Return), rhs(rhs)
		{ }
		const ValuePtr& getRhs() const { return rhs; }
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class PushInsn : public Insn {
		ValuePtr rhs;
	public:
		PushInsn(const ValuePtr& rhs) :
			Insn(IC_Stack, IT_Push), rhs(rhs)
		{ }
		unsigned getNumOfBytes() const { return 4; }
		const ValuePtr& getRhs() const { return rhs; }
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class PopInsn : public Insn {
		ValuePtr rhs;
	public:
		PopInsn(const ValuePtr& rhs) :
			Insn(IC_Stack, IT_Pop), rhs(rhs)
		{ }
		unsigned getNumOfBytes() const;
		const ValuePtr& getRhs() const { return rhs; }
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class CallInsn : public Insn {
		FunctionPtr callee;
		VariablePtr result;
	public:
		CallInsn(const FunctionPtr& callee) :
			Insn(IC_Call, IT_Call), callee(callee) {
			assert(callee && "callee must not be null");
		}
		CallInsn(const FunctionPtr& callee, const VariablePtr& result);
		const FunctionPtr& getCallee() const { return callee; }
		const VariablePtr& getResult() const { return result; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class AllocaInsn : public Insn {
		ValuePtr size;
		VariablePtr variable;
		ValueList dimensions;
	public:
		AllocaInsn(const VariablePtr& variable, const ValuePtr& size, const ValueList& dimensions);
		const ValuePtr& getSize() const { return size; }
		const VariablePtr& getVariable() const { return variable; }
		const ValueList& getDimensions() const { return dimensions; }
		bool isConst() const { return size->getValueCategory() == Value::VC_Constant; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class LoadInsn : public Insn {
		VariablePtr source;
		VariablePtr target;
	public:
		LoadInsn(const VariablePtr& source, const VariablePtr& target) :
			Insn(IC_Assign, IT_Load), source(source), target(target) {
			assert(source && "source must not be null");
			assert(target && "target must not be null");
		}
		const VariablePtr& getSource() const { return source; }
		void setSource(const VariablePtr& source) { this->source = source; }
		const VariablePtr& getTarget() const { return target; }
		void setTarget(const VariablePtr& target) { this->target = target; }
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class StoreInsn : public Insn {
		ValuePtr source;
		VariablePtr target;
	public:
		StoreInsn(const ValuePtr& source, const VariablePtr& target) :
			Insn(IC_Assign, IT_Store), source(source), target(target) {
			assert(source && "source must not be null");
			assert(target && "target must not be null");
		}
		const ValuePtr& getSource() const { return source; }
		void setSource(const ValuePtr& source) { this->source = source; }
		const VariablePtr& getTarget() const { return target; }
		void setTarget(const VariablePtr& target) { this->target = target; }
		void replaceNode(const NodePtr& target, const NodePtr& replacement) override;
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class PushSpInsn : public Insn {
		VariablePtr rhs;
	public:
		PushSpInsn(const VariablePtr& rhs) :
			Insn(IC_Stack, IT_PushSp), rhs(rhs) {
			assert(rhs && "rhs must not be null");
			assert(rhs->getType()->isInt() && "rhs must be of type int");
		}
		const VariablePtr& getRhs() const { return rhs; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class PopSpInsn : public Insn {
		VariablePtr rhs;
	public:
		PopSpInsn(const VariablePtr& rhs) :
			Insn(IC_Stack, IT_PopSp), rhs(rhs) {
			assert(rhs && "rhs must not be null");
			assert(rhs->getType()->isInt() && "rhs must be of type int");
		}
		const VariablePtr& getRhs() const { return rhs; }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	};

	class BasicBlock : public Printable {
		LabelInsnPtr lbl;
		InsnList insns;
		FunctionPtr parent;
	public:
		const LabelInsnPtr& getLabel() const { return lbl; }
		void setLabel(const LabelInsnPtr& lbl) { this->lbl = lbl; }
		const InsnList& getInsns() const { return insns; }
		bool isValid() const;
		bool hasParent() const { return parent != nullptr; }
		const FunctionPtr& getParent() const { assert(hasParent() && "bb has no parent"); return parent; }
		void setParent(const FunctionPtr& parent) { this->parent = parent; }
		bool operator <(const BasicBlock& other) const;
		bool operator ==(const BasicBlock& other) const;
		bool operator !=(const BasicBlock& other) const { return !(*this == other); }
		std::ostream& printTo(std::ostream& stream) const override;
		static void append(const BasicBlockPtr& bb, const InsnPtr& insn) {
			bb->insns.push_back(insn);
			insn->setParent(bb);
		}
		static void prepend(const BasicBlockPtr& bb, const InsnPtr& insn) {
			bb->insns.insert(bb->insns.begin(), insn);
			insn->setParent(bb);
		}
		static InsnList::iterator remove(const BasicBlockPtr& bb, const InsnList::const_iterator& it) {
			return bb->insns.erase(it);
		}
	};

	class Function : public Node {
	public:
		Function(const LabelInsnPtr& label, const FunctionTypePtr& type, const VariableList& parameters) :
			Node(Node::NC_Function), label(label), type(type), parameters(parameters) {}
		std::string getName() const { return getLabel()->getName(); }
		const LabelInsnPtr& getLabel() const { return label; }
		const FunctionTypePtr& getType() const { return type; }
		const VariableList& getParameters() const { return parameters; }
		DirectedGraph<BasicBlock>& getGraph() { return graph; }
		const DirectedGraph<BasicBlock>& getGraph() const { return graph; }
		EdgeList& getEdges() { return graph.getEdges(); }
		const EdgeList& getEdges() const { return graph.getEdges(); }
		BasicBlockList& getBasicBlocks() { return graph.getVertices(); }
		const BasicBlockList& getBasicBlocks() const { return graph.getVertices(); }
		bool operator==(const Node& other) const override;
		std::ostream& printTo(std::ostream& stream) const override;
	private:
		LabelInsnPtr label;
		FunctionTypePtr type;
		VariableList parameters;
		DirectedGraph<BasicBlock> graph;
	};

	class Program : public Node {
	public:
		Program() : Node(Node::NC_Program) {}
		void addFunction(const FunctionPtr& fun) { functions.push_back(fun); }
		FunctionList& getFunctions() { return functions; }
		const FunctionList& getFunctions() const { return functions; }
		bool operator==(const Node& other) const override;
	private:
		FunctionList functions;
	};

	class NodeManager : public Printable {
	public:
		NodeManager() : tmpNr(0), lblNr(0), program(std::make_shared<Program>()) { }
		TypePtr buildBasicType(Type::TypeId typeId);
		ArrayTypePtr buildArrayType(const TypePtr& elementType, const unsigned numOfDims);
		FunctionTypePtr buildFunctionType(const TypePtr& returnType, const TypeList& parameterTypes);

		VariablePtr buildVariable(const TypePtr& type, const std::string& name);
		VariablePtr buildOffset(const TypePtr& type);
		VariablePtr buildTemporary(const TypePtr& type);
		ValuePtr buildIntConstant(int value);
		ValuePtr buildFloatConstant(float value);
		AssignInsnPtr buildAssign(const VariablePtr& lhs, const ValuePtr& rhs1);
		AssignInsnPtr buildAssign(AssignInsn::OpType op, const VariablePtr& lhs, const ValuePtr& rhs1);
		AssignInsnPtr buildAssign(AssignInsn::OpType op, const VariablePtr& lhs, const ValuePtr& rhs1, const ValuePtr& rhs2);
		LabelInsnPtr buildLabel();
		GotoInsnPtr buildGoto(const LabelInsnPtr& target);
		PhiInsnPtr buildPhi(const VariablePtr& lhs, const VariableList& rhs);
		FalseJumpInsnPtr buildFalseJump(const ValuePtr& cond, const LabelInsnPtr& target);
		ReturnInsnPtr buildReturn();
		ReturnInsnPtr buildReturn(const ValuePtr& value);
		PushInsnPtr buildPush(const ValuePtr& value);
		PopInsnPtr buildPop(const ValuePtr& value);
		PopInsnPtr buildPop(size_t numOfBytes);
		CallInsnPtr buildCall(const FunctionPtr& callee);
		CallInsnPtr buildCall(const FunctionPtr& callee, const VariablePtr& result);
		FunctionPtr buildFunction(const std::string& name, const FunctionTypePtr& type, const VariableList& parameters);
		AllocaInsnPtr buildAlloca(const VariablePtr& variable, const ValuePtr& size);
		AllocaInsnPtr buildAlloca(const VariablePtr& variable, const ValuePtr& size, const ValueList& dimensions);
		LoadInsnPtr buildLoad(const VariablePtr& source, const VariablePtr& target);
		StoreInsnPtr buildStore(const ValuePtr& source, const VariablePtr& target);
		PushSpInsnPtr buildPushSp(const VariablePtr& rhs);
		PopSpInsnPtr buildPopSp(const VariablePtr& rhs);

		ProgramPtr getProgram() const { return program; }
		std::ostream& printTo(std::ostream& stream) const override;
	private:
		std::string getUniqueTemporaryName();
		std::string getUniqueLabelName();

		unsigned tmpNr;
		unsigned lblNr;
		ProgramPtr program;
		InstanceManager<Value> values;
		InstanceManager<Type> types;
	};

	bool dumpTo(const ProgramPtr& program, const std::string& dir);
}
namespace std {
	template<>
	struct hash<core::Type> {
		size_t operator()(const core::Type& type) const {
			return static_cast<size_t>(type.getTypeId());
		}
	};

	template<>
	struct hash<core::Value> {
		size_t operator()(const core::Value& value) const {
			return combine_hash(hash<core::Type>()(*value.getType()),
				static_cast<size_t>(value.getValueCategory()),
				static_cast<size_t>(value.getValueType()));
		}
	};
}
