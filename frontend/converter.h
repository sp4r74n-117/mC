#pragma once
#include "core/analysis/analysis.h"
#include "frontend/ast.h"
#include "frontend/parser.h"

namespace frontend {

	using namespace core;

	class Converter {
	public:
		Converter(NodeManager& manager, const std::string& fragment) :
			manager(manager), root(parser::parse(fragment)), varNr(0)
		{ }
		Converter(NodeManager& manager, sptr<ast::node> root) :
			manager(manager), root(root), varNr(0)
		{ }
		void convert();
		const InsnList& getInsns() const { return instructions; }
	private:
		std::string getUniqueVariableName(const std::string& name);
		TypePtr generateType(const sptr<ast::type>& type);
		VariablePtr generateVariable(const sptr<ast::variable>& var);
		ValuePtr generateLiteral(const sptr<ast::literal>& var);
		AssignInsn::OpType generateOp(const sptr<ast::unary_operand>& op);
		AssignInsn::OpType generateOp(const sptr<ast::binary_operand>& op);
		ValuePtr generateUnary(const sptr<ast::unary_operation>& unary);
		ValuePtr generateBinary(const sptr<ast::binary_operation>& binary);
		ValuePtr generateCall(const sptr<ast::call_expr>& expr);
		ValuePtr generateExpr(const sptr<ast::expression>& expr);
		ValuePtr generateLoadIfNeeded(const ValuePtr& value);
		ValuePtr generateSub(const sptr<ast::subscript_operation>& sub);
		ValuePtr generateOffset(const sptr<ast::subscript_operation>& sub);
		void generateDecl(const sptr<ast::var_decl_stmt>& decl);
		void generateIf(const sptr<ast::if_stmt>& stmt);
		void generateWhile(const sptr<ast::while_stmt>& stmt);
		void generateFor(const sptr<ast::for_stmt>& stmt);
		void generateRet(const sptr<ast::return_stmt>& stmt);
		void generateComp(const sptr<ast::compound_stmt>& comp);
		void convert(sptr<ast::node> tree);
		void convertProgram(sptr<ast::program> program);
		void convertFragment(sptr<ast::node> node);
		void buildBasicBlocks(const FunctionPtr& fun);

		NodeManager& manager;
		sptr<ast::node> root;

		unsigned varNr;
		InsnList instructions;
		ProgramPtr program; // valid iff convertProgram is used
		VariablePtr pushSp; // valid iff the outermost compound is dyn_stack
		std::map<sptr<ast::variable>, VariablePtr> vars;
	};
}
