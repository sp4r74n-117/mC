#pragma once

#include "basics.h"

#include <vector>
#include <map>
#include <iostream>

namespace ast {

	// abstract bases //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct node {
		int row; // valid if >= 0
		node() : row(-1) {}
		virtual ~node() {}
		virtual bool operator==(const node& other) const = 0;
		virtual std::ostream& print_to(std::ostream& stream) const = 0;

		bool operator!=(const node& other) const;
	};

	struct type : public node {};

	struct expression : public node {};
	struct literal : public expression {};

	struct statement : public node {};

	// lists ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct variable;
	struct function;
	using node_list = std::vector<sptr<node>>;
	using expr_list = std::vector<sptr<expression>>;
	using stmt_list = std::vector<sptr<statement>>;
	using type_list = std::vector<sptr<type>>;
	using vars_list = std::vector<sptr<variable>>;
	using funs_list = std::vector<sptr<function>>;

	bool operator==(const node_list& lhs, const node_list& rhs);
	bool operator==(const expr_list& lhs, const expr_list& rhs);
	bool operator==(const stmt_list& lhs, const stmt_list& rhs);
	bool operator==(const type_list& lhs, const type_list& rhs);
	bool operator==(const vars_list& lhs, const vars_list& rhs);
	bool operator==(const funs_list& lhs, const funs_list& rhs);

	// TYPES ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct int_type : public type {
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct float_type : public type {
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct void_type : public type {
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct array_type : public type {
		sptr<type> element_type;
		unsigned dimensions;
		array_type(const sptr<type>& element_type, unsigned dimensions) :
			element_type(element_type), dimensions(dimensions) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct function_type : public type {
		sptr<type> return_type;
		type_list parameter_types;
		function_type(const sptr<type>& return_type, const type_list& parameter_types) : return_type(return_type), parameter_types(parameter_types) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	// EXPRESSIONS /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// terminals ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct int_literal : public literal {
		int value;
		int_literal(int value) : value(value) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};
	struct float_literal : public literal {
		float value;
		float_literal(float value) : value(value) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct variable : public expression {
		sptr<type> var_type;
		string name;
		variable(sptr<type> var_type, string name) : var_type(var_type), name(name) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct array : public variable {
		expr_list dimensions;
		array(const sptr<type>& arr_type, const std::string& name, const expr_list& dimensions) :
			variable(arr_type, name), dimensions(dimensions) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct function_decl : public node {
		std::string name;
		sptr<function_type> type;
		vars_list params;
		function_decl(const std::string& name, const sptr<function_type>& type, const vars_list& params) : name(name), type(type), params(params) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct function : public node {
		sptr<function_decl> decl;
		sptr<statement> body;
		function(const sptr<function_decl>& decl, const sptr<statement>& body) : decl(decl), body(body) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct program : public node {
		funs_list funs;
		program(const funs_list& funs) : funs(funs) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	enum class binary_operand {
		ASSIGN, ADD, SUB, MUL, DIV,
		EQ, NE, LT, GT, LE, GE
	};

	static const std::map<string, ast::binary_operand> binary_operand_map {
		{"+", ast::binary_operand::ADD}, {"-", ast::binary_operand::SUB},
		{"*", ast::binary_operand::MUL}, {"/", ast::binary_operand::DIV},
		{"==", ast::binary_operand::EQ}, {"!=", ast::binary_operand::NE},
		{"<=", ast::binary_operand::LE}, {">=", ast::binary_operand::GE},
		{"<" , ast::binary_operand::LT}, {">" , ast::binary_operand::GT},
		{"=", ast::binary_operand::ASSIGN}
	};

	enum class unary_operand {
		MINUS,
		NOT
	};

	// non-terminals ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct subscript_operation : public expression {
		sptr<variable> var;
		expr_list exprs;
		subscript_operation(const sptr<variable>& var, const expr_list& exprs) :
			var(var), exprs(exprs) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct binary_operation : public expression {
		sptr<binary_operand> op;
		sptr<expression> lhs;
		sptr<expression> rhs;
		binary_operation(sptr<binary_operand> op, sptr<expression> lhs, sptr<expression> rhs) : op(op), lhs(lhs), rhs(rhs) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct unary_operation : public expression {
		sptr<unary_operand> op;
		sptr<expression> sub;
		unary_operation(sptr<unary_operand> op, sptr<expression> sub) : op(op), sub(sub) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct paren_expr : public expression {
		sptr<expression> sub;
		paren_expr(sptr<expression> sub) : sub(sub) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct call_expr : public expression {
		sptr<function> fun;
		expr_list args;
		call_expr(const sptr<function>& fun, const expr_list& args) : fun(fun), args(args) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	// STATEMENTS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct expr_stmt : public statement {
		sptr<expression> sub;
		expr_stmt(sptr<expression> sub) : sub(sub) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct compound_stmt : public statement {
		stmt_list statements;
		bool dyn_stack;
		compound_stmt(const stmt_list& statements) : statements(statements), dyn_stack(false) {}
		compound_stmt() {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct if_stmt : public statement {
		sptr<expression> condition;
		sptr<statement> then_stmt;
		sptr<statement> else_stmt;
		if_stmt(sptr<expression> condition, sptr<statement> then_stmt, sptr<statement> else_stmt)
			: condition(condition), then_stmt(then_stmt), else_stmt(else_stmt) {}
		if_stmt(sptr<expression> condition, sptr<statement> then_stmt) : if_stmt(condition, then_stmt, std::make_shared<compound_stmt>()) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct var_decl_stmt : public statement {
		sptr<variable> var;
		sptr<expression> init_expr;
		var_decl_stmt(sptr<variable> var, sptr<expression> init_expr) : var(var), init_expr(init_expr) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct fun_decl_stmt : public statement {
		sptr<function_decl> decl;
		fun_decl_stmt(const sptr<function_decl>& decl) : decl(decl) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct call_stmt : public statement {
		sptr<call_expr> expr;
		call_stmt(const sptr<call_expr>& expr) : expr(expr) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct while_stmt : public statement {
		sptr<expression> condition;
		sptr<statement> body;

		while_stmt(sptr<expression> condition, sptr<statement> body) : condition(condition), body(body) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct for_stmt : public statement {
		stmt_list init;
		sptr<expression> condition;
		stmt_list iteration;
		sptr<statement> body;

		for_stmt(stmt_list init, sptr<expression> condition, stmt_list iteration, sptr<statement> body) :
			init(init), condition(condition), iteration(iteration), body(body)
		{ }
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};

	struct return_stmt : public statement {
		sptr<expression> expr;
		return_stmt(const sptr<expression>& expr) : expr(expr) {}
		virtual bool operator==(const node& other) const;
		virtual std::ostream& print_to(std::ostream& stream) const;
	};
}

bool operator==(const sptr<ast::node>& lhs, const sptr<ast::node>& rhs);

std::ostream& operator<<(std::ostream& stream, const ast::node& node);

template<class T>
std::ostream& operator<<(std::ostream& stream, const sptr<T>& node) {
	if(!node) return stream << "NULL";
	return stream << *node;
}
