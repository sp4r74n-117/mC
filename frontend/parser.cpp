#include "parser.h"

#include <type_traits>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <cassert>
#include "utils/utils-mangle.h"
#include "string_utils.h"

// helper functions ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
	using namespace parser;

	bool is_keyword(const std::string& s) {
		static std::unordered_set<std::string> builtins = {
			"int", "void", "float", "if", "else", "for", "while", "return"
		};
		return builtins.find(s) != builtins.end();
	}

	void consume_whitespace(parser_state& p) {
		auto& s = p.s; auto e = p.e;
		while(s != e && (*s == ' ' || *s == '\t' || *s == '\n')) ++s;
	}

	bool is_identifier_start(char c) {
		return isalpha(c) || c=='_';
	}
	bool is_identifier_char(char c) {
		return is_identifier_start(c) || isdigit(c);
	}

	string consume_identifier(parser_state& p) {
		auto try_p = p;
		consume_whitespace(try_p);
		auto ident_start = try_p.s;
		if(try_p.s == try_p.e || !is_identifier_start(*try_p.s++)) return {};
		while(try_p.s != try_p.e && is_identifier_char(*try_p.s)) try_p.s++;
		string ret(ident_start, try_p.s);
		p = try_p;
		consume_whitespace(p);
		return ret;
	}

	template<class Last>
	string try_token(parser_state& p, Last last_in) {
		auto try_p = p;
		consume_whitespace(try_p);
		string last{last_in};
		auto string_it = last.cbegin();
		while(try_p.s != try_p.e && string_it != last.cend() && *try_p.s == *string_it) {
			try_p.s++;
			string_it++;
		}
		if(string_it == last.cend()) {
			p = try_p;
			consume_whitespace(p);
			return last;
		}
		return {};
	}
	template<class First, class ...Tries>
	string try_token(parser_state& p, First first, Tries... tries) {
		auto match = try_token(p, first);
		if(!match.empty()) return match;
		return try_token(p, tries...);
	}

	template<class Production, class Last>
	Production try_match(parser_state& p, Last last) {
		auto try_p = p;
		consume_whitespace(try_p);
		auto last_res = last(try_p);
		if(last_res) {
			p = try_p;
			consume_whitespace(p);
			return last_res;
		}
		return {};
	}
	template<class Production, class First, class ...Tries>
	Production try_match(parser_state& p, First first, Tries... tries) {
		auto match = try_match<Production, First>(p, first);
		if(match) return match;
		return try_match<Production>(p, tries...);
	}

	template<class Parser, class RetType = typename std::result_of<Parser(parser_state&)>::type>
	RetType expect(Parser pp, parser_state& p) {
		RetType ret = pp(p);
		if(!ret) {
			std::stringstream ss;
			ss << "Expected " << typeid(RetType).name();
			throw parser_error(p, ss.str());
		}
		return ret;
	}

	void report_parser_error(const parser_error& err) {
		const auto& p = err.state;
		// find beginning and end of error line
		auto ls = p.s;
		size_t char_index = 0;
		while(ls != p.beginning && *ls != '\n') {
			char_index += (*ls == '\t') ? 4 : 1;
			ls--;
		}
		auto le = p.s;
		while(le != p.e && *le != '\n') {
			le++;
		}
		// find line number of error
		auto line_num = std::count(p.beginning, ls, '\n') + 2;
		std::cerr << "Parsing error on line number " << line_num << ", column " << char_index << ":" << std::endl;
		auto line = string(ls+1, le);
		replace_all(line, "\t", "    ");
		std::cerr << "Context:\n" << line << std::endl;
		for(size_t i=0; i < char_index; ++i) std::cerr << " ";
		std::cerr << "^" << std::endl;
		std::cerr << "Message: " << err.message << std::endl;
	}
}


namespace parser {

	// state ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void scope::declare(const parser_state& p, string name, sptr<ast::variable> var) {
		if(storage.find(name) != storage.end()) throw parser_error(p, "Declaring variable with name which already exists in this scope");
		// check for the case where we would shadow an argument of the current function
		if (p.fun) {
			for (const auto& var : p.fun->decl->params) {
				if (var->name == name) throw parser_error(p, "Declaring variable with name which already exists in function head");
			}
		}
		storage[name] = var;
	}

	sptr<ast::variable> scope::lookup(string name) const {
		auto it = storage.find(name);
		if(it == storage.end()) return{};
		return it->second;
	}

	void functions::declare(const parser_state& p, string name, sptr<ast::function> fun) {
		if(storage.find(name) != storage.end()) throw parser_error(p, "Declaring function with name which already exists");
		storage[name] = fun;
	}

	sptr<ast::function> functions::lookup(string name) const {
		auto it = storage.find(name);
		if(it == storage.end()) return{};
		return it->second;
	}

	sptr<ast::variable> lookup_variable(const parser_state& state, string name) {
		for(auto it = state.scopes.crbegin(); it != state.scopes.crend(); ++it) {
			auto var = it->lookup(name);
			if(var) return var;
		}
		return {};
	}

	parser_state::parser_state(string::const_iterator s, string::const_iterator e) : beginning(s), s(s), e(e) {
		scopes.push_back(scope());
	}

	void parser_state::set_string(const string& to_parse) {
		s = to_parse.cbegin();
		e = to_parse.cend();
	}

	// parsing /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sptr<ast::node> parse(const string& input) {
		parser_state s{input.cbegin(), input.cend()};
		try {
			auto node = try_match<sptr<ast::node>>(s, program, statement, expression);
			consume_whitespace(s);
			if (s.s != s.e) throw parser_error(s, "Unexpected tokens at end of file");
			return node;
		} catch(const parser_error& err) {
			report_parser_error(err);
			return {};
		}
	}

	sptr<ast::expression> parse_expr(const string& input) {
		parser_state s{input.cbegin(), input.cend()};
		return expression(s);
	}

	sptr<ast::type> type(parser_state& p) {
		auto t = try_token(p, "int", "float");
		if(!t.empty()) {
			if(t == "int") return std::make_shared<ast::int_type>();
			if(t == "float") return std::make_shared<ast::float_type>();
		}
		return {};
	}

	sptr<ast::type> type_or_void(parser_state& p) {
		auto t = type(p);
		if (!t) {
			// in this case check for the presence of void
			if (!try_token(p, string("void")).empty()) return std::make_shared<ast::void_type>();
		}
		return t;
	}

	sptr<ast::int_literal> int_literal(parser_state& p) {
		auto try_p = p;
		while(try_p.s != try_p.e && isdigit(*try_p.s)) ++try_p.s;
		if(try_p.s != p.s) {
			auto ret = std::make_shared<ast::int_literal>(atoi(string{p.s,try_p.s}.c_str()));
			p = try_p;
			return ret;
		}
		return {};
	}

	sptr<ast::float_literal> float_literal(parser_state& p) {
		auto try_p = p;
		while(try_p.s != try_p.e && isdigit(*try_p.s)) ++try_p.s;
		if(try_p.s != try_p.e && *try_p.s == '.') ++try_p.s;
		else return {};
		while(try_p.s != try_p.e && isdigit(*try_p.s)) ++try_p.s;
		if(try_p.s != p.s) {
			// in case we face 'f', consume that as well
			if (*try_p.s == 'f') ++try_p.s;
			auto ret = std::make_shared<ast::float_literal>(atof(string{p.s,try_p.s}.c_str()));
			p = try_p;
			return ret;
		}
		return {};
	}

	sptr<ast::variable> variable(parser_state& p) {
		auto try_p = p;
		auto identifier = consume_identifier(try_p);
		if(identifier.empty()) return {};
		auto var = lookup_variable(try_p, identifier);
		if(!var) return {};
		p = try_p;
		return var;
	}

	sptr<ast::literal> literal(parser_state& p) {
		return try_match<sptr<ast::literal>>(p, float_literal, int_literal);
	}

	sptr<ast::binary_operand> binary_operand(parser_state& p) {
		auto t = try_token(p, "+", "-", "*", "/", "==", "!=", "<=", ">=", "<", ">", "=");
		if(!t.empty()) {
			return std::make_shared<ast::binary_operand>(ast::binary_operand_map.find(t)->second);
		}
		return {};
	}

	sptr<ast::unary_operand> unary_operand(parser_state& p) {
		if(p.s == p.e) return{};
		sptr<ast::unary_operand> ret;
		switch(*p.s) {
		case '-': ret = std::make_shared<ast::unary_operand>(ast::unary_operand::MINUS); break;
		case '!': ret = std::make_shared<ast::unary_operand>(ast::unary_operand::NOT); break;
		}
		if(ret) p.s++;
		return ret;
	}

	sptr<ast::binary_operation> binary_operation(parser_state& p) {
		auto try_p = p;
		auto lhs = single_expression(try_p);
		if(!lhs) return {};
		consume_whitespace(try_p);
		auto op = binary_operand(try_p);
		if(!op) return {};
		consume_whitespace(try_p);
		auto rhs = expect(expression, try_p);
		if(!rhs) return {};
		p = try_p;
		return std::make_shared<ast::binary_operation>(op, lhs, rhs);
	}

	sptr<ast::unary_operation> unary_operation(parser_state& p) {
		auto try_p = p;
		auto op = unary_operand(try_p);
		if(!op) return {};
		consume_whitespace(try_p);
		auto sub = expect(expression, try_p);
		if(!sub) return {};
		p = try_p;
		return std::make_shared<ast::unary_operation>(op, sub);
	}

	sptr<ast::subscript_operation> subscript_operation(parser_state& p) {
		auto try_p = p;
		auto var = variable(try_p);
		if(!var) return {};

		auto is_lbracket = [&]() {
			consume_whitespace(try_p);
			return *try_p.s == '[';
		};
		if(!is_lbracket()) return {};

		// now to the looping
		ast::expr_list exprs;
		do {
			if(try_token(try_p, string("[")).empty()) throw parser_error(try_p, "Expected '[' within array decl");
			auto expr = expect(expression, try_p);
			exprs.push_back(expr);
			if(try_token(try_p, string("]")).empty()) throw parser_error(try_p, "Expected ']' within array decl");
		} while (is_lbracket());
		// fixup parser state
		p = try_p;

		auto arr = std::dynamic_pointer_cast<ast::array>(var);
		if (!arr) throw parser_error(p, "Subscript may only be used on arrays");
		if (exprs.size() != arr->dimensions.size()) throw parser_error(p, "Subscript operation must result in a primitive");

		auto res = std::make_shared<ast::subscript_operation>(var, exprs);
		res->row = std::count(p.beginning, p.s, '\n') + 1;
		return res;
	}

	sptr<ast::paren_expr> paren_expr(parser_state& p) {
		if(try_token(p, string("(")).empty()) return {};
		auto sub = expect(expression, p);
		if(try_token(p, string(")")).empty()) throw parser_error(p, "Expected ')' at the end of parenthesized expression");
		return std::make_shared<ast::paren_expr>(sub);
	}

	sptr<ast::expression> single_expression(parser_state& p) {
		return try_match<sptr<ast::expression>>(p, literal, subscript_operation, variable, unary_operation, paren_expr, call_expr);
	}

	sptr<ast::expression> expression(parser_state& p) {
		return try_match<sptr<ast::expression>>(p, binary_operation, single_expression);
	}

	sptr<ast::expr_stmt> expr_stmt(parser_state& p) {
		auto expr = expression(p);
		if(expr) {
			if(try_token(p, ";").empty()) throw parser_error(p, "Expected ';' at end of statement");
			return std::make_shared<ast::expr_stmt>(expr);
		}
		return {};
	}

	sptr<ast::compound_stmt> compound_stmt(parser_state& p) {
		if(try_token(p, "{").empty()) return {};
		p.scopes.push_back(scope());
		bool dyn_stack = false;
		ast::stmt_list statements;
		while(auto stmt = statement(p)) {
			if (!dyn_stack) {
				if (auto decl = std::dynamic_pointer_cast<ast::var_decl_stmt>(stmt)) {
					if (auto array = std::dynamic_pointer_cast<ast::array>(decl->var)) {
						// check if one of the dimensions is not a constant
						for (const auto& dim : array->dimensions) {
							auto lit = std::dynamic_pointer_cast<ast::int_literal>(dim);
							if (!lit) { dyn_stack = true; break; }
						}
					}
				}
			}
			statements.push_back(stmt);
		}
		p.scopes.pop_back();
		if(try_token(p, "}").empty()) throw parser_error(p, "Expected '}' at end of compound statement");

		auto res = std::make_shared<ast::compound_stmt>(statements);
		res->dyn_stack = dyn_stack;
		return res;
	}

	sptr<ast::if_stmt> if_stmt(parser_state& p) {
		if(try_token(p, "if").empty()) return {};
		auto condition = expect(paren_expr, p)->sub;
		auto then_stmt = expect(statement, p);
		if(!try_token(p, "else").empty()) {
			auto else_stmt = expect(statement, p);
			return std::make_shared<ast::if_stmt>(condition, then_stmt, else_stmt);
		}
		return std::make_shared<ast::if_stmt>(condition, then_stmt);
	}

	sptr<ast::while_stmt> while_stmt(parser_state& p) {
		if(try_token(p, "while").empty()) return {};
		auto condition = expect(paren_expr, p)->sub;
		auto body = expect(statement, p);
		return std::make_shared<ast::while_stmt>(condition, body);
	}

	sptr<ast::var_decl_stmt> var_decl_stmt(parser_state& p) {
		auto var_type = type(p);
		if(!var_type) return {};
		auto id = consume_identifier(p);
		if(id.empty()) throw parser_error(p, "Expected identifier in variable declaration");
		auto var = std::make_shared<ast::variable>(var_type, id);
		auto init_eq = try_token(p, "=");
		sptr<ast::expression> init_expr;
		if(!init_eq.empty()) {
			init_expr = expect(expression, p);
		}
		if(try_token(p, ";").empty()) throw parser_error(p, "Expected ';' at end of statement");
		// store variable in current scope
		p.scopes.back().declare(p, id, var);
		return std::make_shared<ast::var_decl_stmt>(var, init_expr);
	}

	sptr<ast::var_decl_stmt> arr_decl_stmt(parser_state& p) {
		auto try_p = p;
		auto is_lbracket = [&]() {
			consume_whitespace(try_p);
			return *try_p.s == '[';
		};

		auto element_type = type(try_p);
		if(!element_type) return {};
		auto id = consume_identifier(try_p);
		if(id.empty()) throw parser_error(try_p, "Expected identifier in variable declaration");
		// check if we face an array dimension listing
		if (!is_lbracket()) return {};
		// now to the looping
		ast::expr_list dimensions;
		do {
			if(try_token(try_p, string("[")).empty()) throw parser_error(try_p, "Expected '[' within array decl");
			auto expr = expect(expression, try_p);
			dimensions.push_back(expr);
			if(try_token(try_p, string("]")).empty()) throw parser_error(try_p, "Expected ']' within array decl");
		} while (is_lbracket());

		if(try_token(try_p, ";").empty()) throw parser_error(try_p, "Expected ';' at end of statement");
		// fixup the parser state
		p = try_p;
		// put together the actual type
		auto arr_type = std::make_shared<ast::array_type>(element_type, dimensions.size());
		auto var = std::make_shared<ast::array>(arr_type, id, dimensions);
		// store variable in current scope
		p.scopes.back().declare(p, id, var);
		return std::make_shared<ast::var_decl_stmt>(var, nullptr);
	}

	sptr<ast::statement> statement(parser_state& p) {
		return try_match<sptr<ast::statement>>(p, if_stmt, arr_decl_stmt, var_decl_stmt, compound_stmt, expr_stmt, while_stmt, for_stmt, return_stmt);
	}

	sptr<ast::for_stmt> for_stmt(parser_state& p) {
		if(try_token(p, "for").empty()) return {};
		// a paren must follow, otherwise the code is malformed!
		if(try_token(p, string("(")).empty()) throw parser_error(p, "Expected '(' after keyword 'for'");
		// local helper to parse: expr0, expr1, ..., exprN
		auto parse_expr_list = [&](ast::stmt_list& result) {
			bool semi = false;
			while (auto expr = try_match<sptr<ast::expression>>(p, binary_operation)) {
				semi = false;
				result.push_back(std::make_shared<ast::expr_stmt>(expr));
				// consume white-space & comma
				consume_whitespace(p);
				if (!try_token(p, string(",")).empty())
					semi = true;
				else
					break;
				consume_whitespace(p);
			}
			if (semi) throw parser_error(p, "Expected expression after ','");
		};

		ast::stmt_list init;
		// valid iff the first element is a decl
		sptr<ast::type> var_type = type(p);
		// create a scope to store the init-clause vars
		p.scopes.push_back(scope());
		// parse the init clause
		consume_whitespace(p);
		// now we have two cases, either we use existing vars or define new ones
		if (var_type) {
			// lhs must always be a variable!
			bool semi = false;
			while (true) {
				auto id = consume_identifier(p);
				if (id.empty()) break;
				semi = false;
				auto var = std::make_shared<ast::variable>(var_type, id);
				if (try_token(p, "=").empty()) throw parser_error(p, "Expected initializer for '" + id + "'");
				auto expr = expect(expression, p);
				// declare the new var within this scope
				p.scopes.back().declare(p, var->name, var);
				// save this stmt for later usage
				init.push_back(std::make_shared<ast::var_decl_stmt>(var, expr));
				// consume white-space & comma
				consume_whitespace(p);
				if (!try_token(p, string(",")).empty())
					semi = true;
				else
					break;
				consume_whitespace(p);
			}
			if (semi) throw parser_error(p, "Expected expression after ','");
		} else {
			// no decls, all variables must be available
			parse_expr_list(init);
		}
		// check end of init-clause
		if(try_token(p, string(";")).empty()) throw parser_error(p, "Expected ';' at the end of init clause");
		// obtain the mandatory condition
		consume_whitespace(p);
		// parse the optional condition, empty in endless loop
		auto condition = try_match<sptr<ast::expression>>(p, expression);
		// check end of condition
		consume_whitespace(p);
		if(try_token(p, string(";")).empty()) throw parser_error(p, "Expected ';' at the end of condition expression");
		// parse all expressions which are processed on each iteration
		ast::stmt_list iteration;
		parse_expr_list(iteration);
		// expect the last paren
		if(try_token(p, string(")")).empty()) throw parser_error(p, "Expected ')' at the end of iteration clause");
		// parse the loop body
		auto body = expect(statement, p);
		// last but not least reduce the scope
		p.scopes.pop_back();
		return std::make_shared<ast::for_stmt>(init, condition, iteration, body);
	}

	sptr<ast::function_decl> function_decl(parser_state& p) {
		auto return_type = type_or_void(p);
		if (!return_type) return {};
		auto name = consume_identifier(p);
		if (name.empty()) throw parser_error(p, "Expected identifier in function declaration");
		// fixup function name
		name = utils::mangle::mangle(name);
		consume_whitespace(p);
		if (try_token(p, string("(")).empty()) return {};

		bool semi = false;
		ast::vars_list parameters;
		ast::type_list parameter_types;
		while (true) {
			auto param_type = type(p);
			if (!param_type) break;

			semi = false;
			// save the type for later on
			parameter_types.push_back(param_type);

			auto id = consume_identifier(p);
			if (!id.empty()) {
				// remember parameter name & check for uniqueness
				auto it = std::find_if(parameters.begin(), parameters.end(),
					[&](const sptr<ast::variable>& var) { return var->name == id; });
				if (it != parameters.end()) throw parser_error(p, "Duplicated parameter name: " + id);

				parameters.push_back(std::make_shared<ast::variable>(param_type, id));
			} else {
				// we must insert a placeholder
				parameters.push_back(std::make_shared<ast::variable>(param_type, ""));
			}

			consume_whitespace(p);
			if (!try_token(p, string(",")).empty())
				semi = true;
			else
				break;
			consume_whitespace(p);
		}

		if (semi) throw parser_error(p, "Expected expression after ','");
		if (try_token(p, string(")")).empty()) throw parser_error(p, "Expected ')' at the end of a function head");

		// put together function type
		auto fun_type = std::make_shared<ast::function_type>(return_type, parameter_types);
		return std::make_shared<ast::function_decl>(name, fun_type, parameters);
	}

	sptr<ast::fun_decl_stmt> fun_decl_stmt(parser_state& p) {
		auto try_p = p;
		auto decl = function_decl(try_p);
		if (!decl) return {};
		// check if it is really a decl, might also be a definition
		if (try_token(try_p, ";").empty()) return {};
		// re-adjust parser state to reflect the parsed node
		p = try_p;
		// also declare a dummy function which the definition picks up later on
		p.funs.declare(p, decl->name, std::make_shared<ast::function>(decl, nullptr));
		return std::make_shared<ast::fun_decl_stmt>(decl);
	}

	sptr<ast::function> function(parser_state& p) {
		auto try_p = p;
		auto decl = function_decl(try_p);
		if (!decl) return {};
		// assure that it is no decl, as otherwise it must be a compound
		if (!try_token(try_p, ";").empty()) return {};
		// fixup parser state
		p = try_p;
		// try to lookup the function in the current parser state
		auto fun = p.funs.lookup(decl->name);
		if (fun) {
			// the function has already been declared thus we need to check types, var names do not matter!
			if (*fun->decl->type->return_type != *decl->type->return_type) throw parser_error(p, "Conflicting return type for: " + decl->name);
			if (fun->decl->params.size() != decl->params.size()) throw parser_error(p, "Conflicting number of parameters for: " + decl->name);

			// now check for equivalence (cannot use list_equal tho!)
			auto it1 = fun->decl->params.begin();
			auto it2 = decl->params.begin();
			for (; it2 != decl->params.begin(); ++it1, ++it2) {
				if (*(*it1)->var_type != *(*it2)->var_type) throw parser_error(p, "Conflicting parameter types for: " + decl->name);
			}

			// lastly, update the previous decl with the new info
			fun->decl = decl;
		}

		// create fresh new scope for function
		p.scopes.clear();
		p.scopes.push_back(scope());
		for (const auto& var : decl->params) {
			// anonymous parameters are ignored in the scope
			if (var->name.empty()) continue;
			p.scopes.back().declare(p, var->name, var);
		}
		// create a function and pre-declare
		if (!fun) {
			// alloc a new one and register within state
			fun = std::make_shared<ast::function>(decl, nullptr);
			p.funs.declare(p, decl->name, fun);
		}
		// adjust the currently processed function
		p.fun = fun;
		auto body = expect(compound_stmt, p);
		// simply replace the placeholder with the concrete one
		fun->body = body;
		p.fun = nullptr;
		return fun;
	}

	sptr<ast::call_expr> call_expr(parser_state& p) {
		auto try_p = p;
		// a function is always an identifier within a call_expr
		auto id = consume_identifier(try_p);
		if (id.empty()) return {};
		// in case we consumed a builtin .. return right along
		if (is_keyword(id)) return {};
		// check for call syntax
		if (try_token(try_p, "(").empty()) return {};
		// now assure that id points to a declared function, otherwise throw
		auto fun = p.funs.lookup(utils::mangle::mangle(id));
		if (!fun) throw parser_error(p, "Call of undeclared function: " + id);
		// fixup parser state, beyond this point we throw in case of error
		p = try_p;
		// parse all passed arguments
		ast::expr_list args;
		bool semi = false;
		while (auto expr = try_match<sptr<ast::expression>>(p, expression)) {
			semi = false;
			args.push_back(expr);
			// consume white-space & comma
			consume_whitespace(p);
			if (!try_token(p, string(",")).empty())
				semi = true;
			else
				break;
			consume_whitespace(p);
		}
		if (semi) throw parser_error(p, "Expected expression after ','");
		// assure correct termination of seq.
		if (try_token(p, ")").empty()) throw parser_error(p, "Expected ')' at the end of a function call");
		if (args.size() != fun->decl->params.size()) throw parser_error(p, "Invalid number of arguments for function call");
		// if the function is actually callable or not is handled in checks.cpp
		return std::make_shared<ast::call_expr>(fun, args);
	}

	sptr<ast::call_stmt> call_stmt(parser_state& p) {
		auto expr = call_expr(p);
		if (!expr) return {};
		if (try_token(p, ";").empty()) throw parser_error(p, "Expected ';' at end of statement");
		return std::make_shared<ast::call_stmt>(expr);
	}

	sptr<ast::return_stmt> return_stmt(parser_state& p) {
		if(try_token(p, "return").empty()) return {};
		auto expr = try_match<sptr<ast::expression>>(p, expression);
		if (expr && p.fun && typeid(*p.fun->decl->type->return_type) == typeid(ast::void_type)) throw parser_error(p, "Cannot return a value from void function");
		if (try_token(p, ";").empty()) throw parser_error(p, "Expected ';' at end of statement");
		return std::make_shared<ast::return_stmt>(expr);
	}

	sptr<ast::program> program(parser_state& p) {
		while (true) {
			auto s = try_match<sptr<ast::node>>(p, fun_decl_stmt, function);
			if (!s) break;
		}

		if (p.funs.empty()) return {};

		ast::funs_list funs;
		p.funs.for_each([&](const sptr<ast::function>& fun) { funs.push_back(fun); });
		return std::make_shared<ast::program>(funs);
	}
}
