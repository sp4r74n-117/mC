#pragma once

#include "basics.h"
#include "ast.h"

namespace parser {

	// main interfaces

	sptr<ast::node> parse(const string& input);
	sptr<ast::expression> parse_expr(const string& input);

	// state

	struct parser_state;

	class scope {
		std::map<string, sptr<ast::variable>> storage;
	public:
		void declare(const parser_state& p, string name, sptr<ast::variable> var);
		sptr<ast::variable> lookup(string name) const;
	};

	class functions {
		std::map<string, sptr<ast::function>> storage;
	public:
		void declare(const parser_state& p, string name, sptr<ast::function> fun);
		sptr<ast::function> lookup(string name) const;
		bool empty() const { return storage.empty(); }
		template<typename TLambda>
		void for_each(TLambda lambda) {
			for (const auto& fun : storage)
				lambda(fun.second);
		}
	};

	struct parser_state {
		string::const_iterator beginning;
		string::const_iterator s;
		string::const_iterator e;
		std::vector<scope> scopes;
		functions funs;
		sptr<ast::function> fun; // current processed function
		parser_state(string::const_iterator s, string::const_iterator e);
		void set_string(const string& to_parse);
	};

	struct parser_error {
		parser_state state;
		string message;
		parser_error(const parser_state& state, const string& message) : state(state), message(message) {}
	};

	// TYPES ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sptr<ast::type> type(parser_state& p);
	sptr<ast::type> type_or_void(parser_state& p);

	// EXPRESSIONS /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// terminals

	sptr<ast::int_literal> int_literal(parser_state& p);
	sptr<ast::float_literal> float_literal(parser_state& p);
	sptr<ast::variable> variable(parser_state& p);
	sptr<ast::binary_operand> binary_operand(parser_state& p);
	sptr<ast::unary_operand> unary_operand(parser_state& p);
	sptr<ast::function_decl> function_decl(parser_state& p);

	// non-terminals

	sptr<ast::literal> literal(parser_state& p);

	sptr<ast::binary_operation> binary_operation(parser_state& p);
	sptr<ast::unary_operation> unary_operation(parser_state& p);
	sptr<ast::paren_expr> paren_expr(parser_state& p);
	sptr<ast::call_expr> call_expr(parser_state& p);
	sptr<ast::expression> single_expression(parser_state& p);
	sptr<ast::subscript_operation> subscript_operation(parser_state& p);
	sptr<ast::expression> expression(parser_state& p);

	// STATEMENTS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sptr<ast::expr_stmt> expr_stmt(parser_state& p);
	sptr<ast::compound_stmt> compound_stmt(parser_state& p);
	sptr<ast::if_stmt> if_stmt(parser_state& p);
	sptr<ast::while_stmt> while_stmt(parser_state& p);
	sptr<ast::var_decl_stmt> var_decl_stmt(parser_state& p);
	sptr<ast::var_decl_stmt> arr_decl_stmt(parser_state& p);
	sptr<ast::for_stmt> for_stmt(parser_state& p);
	sptr<ast::statement> statement(parser_state& p);
	sptr<ast::fun_decl_stmt> fun_decl_stmt(parser_state& p);
	sptr<ast::call_stmt> call_stmt(parser_state& p);
	sptr<ast::return_stmt> return_stmt(parser_state& p);
	sptr<ast::function> function(parser_state& p);
	sptr<ast::program> program(parser_state& p);
}
