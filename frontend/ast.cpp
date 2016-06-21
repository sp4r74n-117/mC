#include "ast.h"

#include <cassert>
#include <iostream>

namespace ast {

	bool node::operator!=(const node& other) const {
		return !(*this == other);
	}

	namespace {
		template<typename T>
		bool list_equal(const T& lhs, const T& rhs) {
			if(lhs.size() != rhs.size()) return false;
			for(size_t i = 0; i < lhs.size(); ++i) {
				if(*lhs[i] != *rhs[i]) return false;
			}
			return true;
		}
	}
	bool operator==(const node_list& lhs, const node_list& rhs) {
		return list_equal(lhs, rhs);
	}
	bool operator==(const expr_list& lhs, const expr_list& rhs) {
		return list_equal(lhs, rhs);
	}
	bool operator==(const stmt_list& lhs, const stmt_list& rhs) {
		return list_equal(lhs, rhs);
	}
	bool operator==(const type_list& lhs, const type_list& rhs) {
		return list_equal(lhs, rhs);
	}
	bool operator==(const vars_list& lhs, const vars_list& rhs) {
		return list_equal(lhs, rhs);
	}
	bool operator==(const funs_list& lhs, const funs_list& rhs) {
		return list_equal(lhs, rhs);
	}

	bool int_type::operator==(const node& other) const {
		return typeid(other) == typeid(int_type);
	}
	std::ostream& int_type::print_to(std::ostream& stream) const {
		return stream << "int";
	}

	bool float_type::operator==(const node& other) const {
		return typeid(other) == typeid(float_type);
	}
	std::ostream& float_type::print_to(std::ostream& stream) const {
		return stream << "float";
	}

	bool void_type::operator==(const node& other) const {
		return typeid(other) == typeid(void_type);
	}

	std::ostream& void_type::print_to(std::ostream& stream) const {
		return stream << "void";
	}

	bool array_type::operator==(const node& other) const {
		return typeid(other) != typeid(array_type) &&
				   *dynamic_cast<const array_type&>(other).element_type == *element_type &&
					 dynamic_cast<const array_type&>(other).dimensions == dimensions;
	}

	std::ostream& array_type::print_to(std::ostream& stream) const {
		element_type->print_to(stream);
		for (unsigned i = 0; i < dimensions; ++i)
			stream << "[]";
		return stream;
	}

	bool function_type::operator==(const node& other) const {
		return typeid(other) == typeid(function_type);
	}

	std::ostream& function_type::print_to(std::ostream& stream) const {
		return_type->print_to(stream);
		stream << "(";
		bool first = true;
		for (const auto& type : parameter_types) {
			if (first) first = false;
			else	   stream << ",";
			type->print_to(stream);
		}
		return stream << ")";
	}

	bool int_literal::operator==(const node& other) const {
		return typeid(other) == typeid(int_literal) && dynamic_cast<const int_literal&>(other).value == value;
	}
	std::ostream& int_literal::print_to(std::ostream& stream) const {
		return stream << value;
	}

	bool float_literal::operator==(const node& other) const {
		return typeid(other) == typeid(float_literal) && dynamic_cast<const float_literal&>(other).value == value;
	}
	std::ostream& float_literal::print_to(std::ostream& stream) const {
		return stream << value;
	}

	bool variable::operator==(const node& other) const {
		if(typeid(other) != typeid(variable)) return false;
		auto o = dynamic_cast<const variable&>(other);
		return *o.var_type == *var_type && o.name == name;
	}
	std::ostream& variable::print_to(std::ostream& stream) const {
		return stream << name;
	}

	bool array::operator==(const node& other) const {
		if(typeid(other) != typeid(array)) return false;
		auto o = dynamic_cast<const array&>(other);
		return *o.var_type == *var_type && o.dimensions == dimensions;
	}

	std::ostream& array::print_to(std::ostream& stream) const {
		auto type = dynamic_cast<const array_type&>(*var_type);
		type.element_type->print_to(stream);
		stream << " " << name;
		for (const auto& dim : dimensions) {
			stream << "[";
			dim->print_to(stream);
			stream << "]";
		}
		return stream;
	}

	std::ostream& operator<<(std::ostream& stream, const binary_operand& op) {
		for(const auto& pair : binary_operand_map) {
			if(pair.second == op) return stream << pair.first;
		}
		assert(false && "Unsupported binary operand");
		return stream;
	}

	bool subscript_operation::operator==(const node& other) const {
		if(typeid(other) != typeid(subscript_operation)) return false;
		auto o = dynamic_cast<const subscript_operation&>(other);
		return *o.var == *var && o.exprs == exprs;
	}

	std::ostream& subscript_operation::print_to(std::ostream& stream) const {
		var->print_to(stream);
		for (const auto& expr : exprs) {
			expr->print_to(stream << "[");
			stream << "]";
		}
		return stream;
	}

	bool binary_operation::operator==(const node& other) const {
		if(typeid(other) != typeid(binary_operation)) return false;
		auto o = dynamic_cast<const binary_operation&>(other);
		return *o.op == *op && *o.lhs == *lhs && *o.rhs == *rhs;
	}
	std::ostream& binary_operation::print_to(std::ostream& stream) const {
		stream << "(";
		lhs->print_to(stream);
		stream << *op;
		rhs->print_to(stream);
		return stream << ")";
	}

	std::ostream& operator<<(std::ostream& stream, const unary_operand& op) {
		switch(op) {
		case unary_operand::MINUS: return stream << "-";
		case unary_operand::NOT: return stream << "!";
		};
		assert(false && "Unsupported unary operand");
		return stream;
	}
	bool unary_operation::operator==(const node& other) const {
		if(typeid(other) != typeid(unary_operation)) return false;
		auto o = dynamic_cast<const unary_operation&>(other);
		return *o.op == *op && *o.sub == *sub;
	}
	std::ostream& unary_operation::print_to(std::ostream& stream) const {
		stream << *op;
		sub->print_to(stream);
		return stream;
	}

	bool paren_expr::operator==(const node& other) const {
		if(typeid(other) != typeid(paren_expr)) return false;
		auto o = dynamic_cast<const paren_expr&>(other);
		return *o.sub == *sub;
	}
	std::ostream& paren_expr::print_to(std::ostream& stream) const {
		stream << "(";
		sub->print_to(stream);
		return stream << ")";
	}

	bool expr_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(expr_stmt)) return false;
		auto o = dynamic_cast<const expr_stmt&>(other);
		return *o.sub == *sub;
	}
	std::ostream& expr_stmt::print_to(std::ostream& stream) const {
		sub->print_to(stream);
		return stream << ";\n";
	}

	bool compound_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(compound_stmt)) return false;
		auto o = dynamic_cast<const compound_stmt&>(other);
		return statements == o.statements;
	}
	std::ostream& compound_stmt::print_to(std::ostream& stream) const {
		stream << "{\n";
		for(auto stmt : statements) {
			stmt->print_to(stream);
		}
		return stream << "}\n";
	}

	bool if_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(if_stmt)) return false;
		auto o = dynamic_cast<const if_stmt&>(other);
		return *o.condition == *condition && *o.then_stmt == *then_stmt && *o.else_stmt == *else_stmt;
	}
	std::ostream& if_stmt::print_to(std::ostream& stream) const {
		stream << "if(";
		condition->print_to(stream);
		stream << ") ";
		then_stmt->print_to(stream);
		stream << "else ";
		return else_stmt->print_to(stream);
	}

	bool while_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(while_stmt)) return false;
		auto o = dynamic_cast<const while_stmt&>(other);
		return *o.condition == *condition && *o.body == *body;
	}
	std::ostream& while_stmt::print_to(std::ostream& stream) const {
		stream << "while";
		condition->print_to(stream);
		body->print_to(stream);
		return stream;
	}

	bool var_decl_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(var_decl_stmt)) return false;
		auto o = dynamic_cast<const var_decl_stmt&>(other);
		return *o.var == *var && *o.init_expr == *init_expr;
	}
	std::ostream& var_decl_stmt::print_to(std::ostream& stream) const {
		var->var_type->print_to(stream);
		stream << " ";
		var->print_to(stream);
		if(init_expr) {
			stream << " = ";
			init_expr->print_to(stream);
		}
		return stream << ";\n";
	}

	bool for_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(for_stmt)) return false;
		auto o = dynamic_cast<const for_stmt&>(other);
		return o.init == init && *o.condition == *condition &&
			o.iteration == iteration && *o.body == *body;
	}
	std::ostream& for_stmt::print_to(std::ostream& stream) const {
		stream << "{\n";
		for (const auto& expr: init)
			expr->print_to(stream);
		stream << "while (";
		if (condition)
			condition->print_to(stream);
		else
			stream << "1";
		stream << ")\n{\n";
		body->print_to(stream);
		for (const auto& iter: iteration)
			iter->print_to(stream);
		return stream << "}\n};\n";
	}

	bool function_decl::operator==(const node& other) const {
		if(typeid(other) != typeid(function_decl)) return false;
		auto o = dynamic_cast<const function_decl&>(other);
		return o.name == name && *o.type == *type && o.params == params;
	}

	std::ostream& function_decl::print_to(std::ostream& stream) const {
		// @TODO
		return stream;
	}

	bool fun_decl_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(fun_decl_stmt)) return false;
		auto o = dynamic_cast<const fun_decl_stmt&>(other);
		return *o.decl == *decl;
	}
	std::ostream& fun_decl_stmt::print_to(std::ostream& stream) const {
		// @TODO
		return stream;
	}

	bool program::operator==(const node& other) const {
		if(typeid(other) != typeid(program)) return false;
		auto o = dynamic_cast<const program&>(other);
		return o.funs == funs;
	}

	std::ostream& program::print_to(std::ostream& stream) const {
		// @TODO
		return stream;
	}

	bool function::operator==(const node& other) const {
		if(typeid(other) != typeid(function)) return false;
		auto o = dynamic_cast<const function&>(other);
		return *o.decl == *decl && o.body && body && *o.body == *body;
	}
	std::ostream& function::print_to(std::ostream& stream) const {
		//@TODO
		return stream;
	}

	bool call_expr::operator==(const node& other) const {
		if(typeid(other) != typeid(call_expr)) return false;
		auto o = dynamic_cast<const call_expr&>(other);
		return *o.fun == *fun && o.args == args;
	}

	std::ostream& call_expr::print_to(std::ostream& stream) const {
		stream << fun->decl->name << "(";
		bool first = true;
		for (const auto& arg : args) {
			if (first) first = false;
			else	   stream << ",";
			arg->print_to(stream);
		}
		return stream << ")";
	}

	bool call_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(call_stmt)) return false;
		auto o = dynamic_cast<const call_stmt&>(other);
		return *o.expr == *expr;
	}

	std::ostream& call_stmt::print_to(std::ostream& stream) const {
		return expr->print_to(stream) << ";";
	}

	bool return_stmt::operator==(const node& other) const {
		if(typeid(other) != typeid(return_stmt)) return false;
		auto o = dynamic_cast<const return_stmt&>(other);
		return *o.expr == *expr;
	}

	std::ostream& return_stmt::print_to(std::ostream& stream) const {
		stream << "return ";
		expr->print_to(stream);
		return stream << ";";
	}
}

bool operator==(const sptr<ast::node>& lhs, const sptr<ast::node>& rhs) {
	return typeid(lhs) == typeid(rhs) && lhs && rhs && *lhs == *rhs;
}

std::ostream& operator<<(std::ostream& stream, const ast::node& node) {
	return node.print_to(stream);
}
