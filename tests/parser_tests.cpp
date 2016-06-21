#include "parser_tests.h"

#include "frontend/parser.h"
#include "test_utils.h"

void test_parser() {
	string str_test{"test"};
	string str_1{"1"};
	string str_4dot5{"4.5"};
	string str_plus{"+"};
	string str_mul{"*"};
	string str_addition{"1+7"};
	string str_add_error{"1+"};
	string str_neg{"-4.2"};
	string str_paren{"(1+7)"};
	string str_expr_stmt{"1;"};
	string str_empty_compound{"{}"};
	string str_compound{"{ 1; { {} 7; }}"};
	string str_if{"if(1) {}"};
	string str_if_else{"if (1) {} else 5;"};
	string str_if_error{"if 7;"};
	string str_int{"int"};
	string str_float{"  float "};
	string str_var_decl{"int test;"};
	string str_fun_decl{"int test(int a, float);"};
	string str_arr_decl{"int test[10];"};

	// types
	EXPECT_NO_MATCH(parser::type, str_plus);
	EXPECT_NO_MATCH(parser::type, str_test);
	EXPECT_MATCH_TYPED(parser::type, str_int, ast::int_type, [](auto res) { return true; });
	EXPECT_MATCH_TYPED(parser::type, str_float, ast::float_type, [](auto res) { return true; });

	// literals
	EXPECT_NO_MATCH(parser::int_literal, str_test);
	EXPECT_NO_MATCH(parser::int_literal, str_plus);
	EXPECT_MATCH(parser::int_literal, str_1, [](auto res) { return res->value == 1; });
	EXPECT_MATCH(parser::float_literal, str_4dot5, [](auto res) { return res->value == 4.5; });

	// variables
	EXPECT_NO_MATCH(parser::variable, str_1);
	EXPECT_NO_MATCH(parser::variable, str_4dot5);
	{
		parser::parser_state p(str_test.cbegin(), str_test.cend());
		sptr<ast::variable> var = std::make_shared<ast::variable>(std::make_shared<ast::int_type>(), str_test);
		p.scopes.back().declare(p, str_test, var);
		// freestanding
		auto match = parser::variable(p);
		EXPECT(match);
		EXPECT(*match == *var);
		EXPECT(p.e == str_test.cend())
			// in expression
			string str_var_expr {
			"test+5"
		};
		p.set_string(str_var_expr);
		auto match2 = parser::binary_operation(p);
		EXPECT(match2);
		EXPECT(*match2->lhs == *var);
		EXPECT(p.e == str_var_expr.cend())
	}

	// binary operands
	EXPECT_NO_MATCH(parser::binary_operand, str_1);
	EXPECT_MATCH(parser::binary_operand, str_plus, [](auto res) { return *res == ast::binary_operand::ADD; });
	EXPECT_MATCH(parser::binary_operand, str_mul, [](auto res) { return *res == ast::binary_operand::MUL; });

	// expressions
	EXPECT_NO_MATCH(parser::literal, str_test);
	EXPECT_MATCH_TYPED(parser::literal, str_1, ast::int_literal, [](auto res) { return res->value == 1; });
	EXPECT_MATCH_TYPED(parser::literal, str_4dot5, ast::float_literal, [](auto res) { return res->value == 4.5; });

	EXPECT_NO_MATCH(parser::binary_operation, str_test);
	EXPECT_NO_MATCH(parser::binary_operation, str_4dot5);
	EXPECT_NO_MATCH(parser::binary_operation, str_mul);
	EXPECT_MATCH(parser::binary_operation, str_addition, [](auto res) { return *res->op == ast::binary_operand::ADD && *res->lhs == ast::int_literal{1}; });
	EXPECT_ERROR(parser::binary_operation, str_add_error);

	EXPECT_NO_MATCH(parser::unary_operation, str_1);
	EXPECT_MATCH(parser::unary_operation, str_neg, [](auto res) { return *res->op == ast::unary_operand::MINUS && *res->sub == ast::float_literal{4.2}; });

	EXPECT_NO_MATCH(parser::paren_expr, str_neg);
	EXPECT_MATCH(parser::paren_expr, str_paren, [](auto res) { return *res->sub == *parser::parse_expr("1+7"); });

	// statements
	EXPECT_ERROR(parser::expr_stmt, str_test);
	EXPECT_MATCH(parser::expr_stmt, str_expr_stmt, [](auto res) { return *res->sub == ast::int_literal{1}; });

	EXPECT_NO_MATCH(parser::compound_stmt, str_test);
	EXPECT_MATCH(parser::compound_stmt, str_empty_compound, [](auto res) { return res->statements.empty(); });
	EXPECT_MATCH(parser::compound_stmt, str_compound, [](auto res) { return res->statements.size() == 2; });

	EXPECT_NO_MATCH(parser::if_stmt, str_test);
	EXPECT_MATCH(parser::if_stmt, str_if, [](auto res) { return *res->condition == ast::int_literal{1}; });
	EXPECT_MATCH(parser::if_stmt, str_if_else, [](auto res) { return *res->else_stmt == ast::expr_stmt{std::make_shared<ast::int_literal>(5)}; });
	EXPECT_MATCH(parser::var_decl_stmt, str_var_decl, [](auto res) { return res->var->name == "test" && typeid(*res->var->var_type) == typeid(ast::int_type); });
	EXPECT_MATCH(parser::fun_decl_stmt, str_fun_decl, [](auto res) {
		if (!res) return false;

		auto decl = res->decl;
		auto type = decl->type;
		return decl->name == "_test" &&
			   typeid(*type->return_type) == typeid(ast::int_type) &&
			   type->parameter_types.size() == 2 &&
			   typeid(*type->parameter_types[0]) == typeid(ast::int_type) &&
			   typeid(*type->parameter_types[1]) == typeid(ast::float_type) &&
			   decl->params.size() == 2 &&
			   decl->params[0]->name == "a" &&
			   decl->params[1]->name == "";});
	EXPECT_MATCH(parser::arr_decl_stmt, str_arr_decl, [](auto res) {
		if (!res) return false;

		auto arr = std::dynamic_pointer_cast<ast::array>(res->var);
		if (!arr) return false;
		if (arr->name != "test") return false;
		if (arr->dimensions.size() != 1) return false;

		auto type = std::dynamic_pointer_cast<ast::array_type>(arr->var_type);
		if (!type) return false;
		return true;
	});

	EXPECT(parser::parse(R"(
	{
		int x;
		if(1 == 2) {
			5+4*8.0;
			7;
			x = 8;
		} else {
			if(2 < 9) {
			}
		}
		2.9;
	}
	)"));

	EXPECT(parser::parse(R"(
	{
		while (1) {}
		while (1 + 2) {}
		for (;;) {}
		for (int a = 0; a < 10; a = a + 2) {}
		for (int a = 0, b = 10; a < b; a = a * 2, b = a / 2) {}
		int a = 0;
		for (a = 2;;) {}
		for (3+4;1/2;4+4) {}
		for (;;a = a + 1) {}
		int b[10];
		float c[a+1];
		int d[12][a+2][13];
	}
	)"));

	EXPECT(parser::parse(R"(
		void a();
		void b(int, int);
		int c(int fst, int snd);
		int d(int, int snd, int);
	)"));

	{
		auto tu = std::dynamic_pointer_cast<ast::program>(
			parser::parse(R"(
				int foo(int, int);
				void bar(int a)
				{
					int c = foo(a + a, 2);
					return;
				}
				int foo(int a, int b)
				{
					return a + b;
				}
			)"));
		EXPECT(tu);
		EXPECT(tu->funs.size() == 2);
		EXPECT(tu->funs[0]->decl->name == "_bar");
		EXPECT(tu->funs[0]->body);
		EXPECT(tu->funs[1]->decl->name == "_foo");
		EXPECT(tu->funs[1]->body);
	}
}
