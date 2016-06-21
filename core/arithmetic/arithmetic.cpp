#include "core/arithmetic/arithmetic.h"
#include "core/analysis/analysis-types.h"
#include "core/analysis/analysis.h"
#include <algorithm>

namespace core {
namespace arithmetic {
	bool isEvaluable(const ValuePtr& lhs, const ValuePtr& rhs) {
		if (!lhs) return false;
		if (!rhs) return false;

		return lhs->getValueCategory() == Value::VC_Constant &&
			   rhs->getValueCategory() == Value::VC_Constant &&
			   (*lhs->getType() == *rhs->getType());
	}

	ValuePtr evaluate(NodeManager& manager, AssignInsn::OpType op, const ValuePtr& lhs, const ValuePtr& rhs) {
		assert(isEvaluable(lhs, rhs) && "expression is not evaluable");
		assert((op != AssignInsn::NONE &&	op != AssignInsn::NOT) && "unary op supplied");
		float result = 0.0;
		switch (op) {
		case AssignInsn::LT:	result = getValue<float>(lhs) < getValue<float>(rhs); break;
		case AssignInsn::GT:	result = getValue<float>(lhs) > getValue<float>(rhs); break;
		case AssignInsn::ADD:	result = getValue<float>(lhs) + getValue<float>(rhs); break;
		case AssignInsn::SUB:	result = getValue<float>(lhs) - getValue<float>(rhs); break;
		case AssignInsn::MUL:	result = getValue<float>(lhs) * getValue<float>(rhs); break;
		case AssignInsn::EQ:	result = getValue<float>(lhs) == getValue<float>(rhs); break;
		case AssignInsn::NE:	result = getValue<float>(lhs) != getValue<float>(rhs); break;
		case AssignInsn::LE:	result = getValue<float>(lhs) <= getValue<float>(rhs); break;
		case AssignInsn::GE:	result = getValue<float>(lhs) >= getValue<float>(rhs); break;
		case AssignInsn::DIV:
			{
				auto div = getValue<float>(rhs);
				if (div == 0)
					// undefined value!
					break;

				result = getValue<float>(lhs) / div;
				break;
			}
		default:
				assert(false && "unsupported op for evaluation");
		}

		if (analysis::types::isInt(lhs->getType()))
			return manager.buildIntConstant(result);
		else
			return manager.buildFloatConstant(result);
	}

	namespace detail {
		int gcd(int a, int b) {
		  int c;
		  while (a != 0) {
		     c = a;
				 a = b%a;
				 b = c;
		  }
		  return b;
		}
	}

	optional<ValuePtr> tryGcd(NodeManager& manager, const ValueList& values) {
		if (values.empty()) return {manager.buildIntConstant(0)};
		if (values.size() == 1) return values[0];

		int result = 0;
		for (unsigned i = 0; i < values.size(); ++i) {
			if (!analysis::isIntConstant(values[i])) return {};
			// assure that we can compute it
			int value = getValue<int>(values[i]);
			if (value == 0) return {manager.buildIntConstant(0)};
			// combine with previous result
			result = i == 0 ? value : detail::gcd(result, value);
		}
		return manager.buildIntConstant(result);
	}

namespace formula {
	TermPtr makeTerm(const ValuePtr& value) {
		auto res = std::make_shared<Term>();
		res->setOp(Term::OpType::NONE);
		res->setValue(value);
		return res;
	}

	TermPtr makeTerm(Term::OpType op, const TermPtr& lhs) {
		auto res = std::make_shared<Term>();
		res->setOp(op);
		res->setLhs(lhs);
		return res;
	}

	TermPtr makeTerm(Term::OpType op, const TermPtr& lhs, const TermPtr& rhs) {
		auto res = std::make_shared<Term>();
		res->setOp(op);
		res->setLhs(lhs);
		res->setRhs(rhs);
		return res;
	}

	TermPtr simplify(NodeManager& manager, const TermPtr& term) {
		if (term->isValue()) return term;

		unsigned children = 0;
		if (auto lhs = term->getLhs()) {
			term->setLhs(simplify(manager, lhs));
			++children;
		}

		if (auto rhs = term->getRhs()) {
			term->setRhs(simplify(manager, rhs));
			++children;
		}

		if (children != 2) return term;

		auto lhs = term->getLhs()->getValue();
		auto rhs = term->getRhs()->getValue();
		if (arithmetic::isEvaluable(lhs, rhs))
			return makeTerm(arithmetic::evaluate(manager, term->getOp(), lhs, rhs));
		else
			return term;
	}

	optional<int> trySolveDiophantine(NodeManager& manager, const TermPtr& lhs, const ValuePtr& rhs) {
		// formula must be in the form: a1*x1 + a2*x2 +... + an*xn = c
		// has an integer solution x1, x2,..., xn iff GCD (a1,a2,.., an) divides c.
		if (!analysis::isIntConstant(rhs)) return {};

		ValueList values;
		collectValues(lhs, values, [&](const ValuePtr& value) {
			if (!analysis::isIntConstant(value)) return false;
			return getValue<int>(value) != 0;
		});
		// use the collected values to compute the global gcd
		auto gcd = arithmetic::tryGcd(manager, values);
		if (!gcd) return {};
		// we cannot devide by zero!
		int value = getValue<int>(*gcd);
		if (!value) return {};
		// check if it has an integer solution
		if ((getValue<int>(rhs) % value) != 0) return {};
		// finally combine the result into an optional
		return {getValue<int>(*gcd)};
	}
}
}
}
namespace utils {
	namespace detail {
		void collectString(const core::arithmetic::formula::Term& term, std::stringstream& ss) {
			if (term.isValue()) {
				term.getValue()->printTo(ss);
				return;
			}

			std::string op;
			switch (term.getOp()) {
			case core::arithmetic::formula::Term::OpType::ADD: op = "+"; break;
			case core::arithmetic::formula::Term::OpType::SUB: op = "-"; break;
			case core::arithmetic::formula::Term::OpType::MUL: op = "*"; break;
			case core::arithmetic::formula::Term::OpType::DIV: op = "/"; break;
			default: op = "?"; break;
			}

			if (!term.getRhs()) ss << op;

			ss << "(";
			collectString(*term.getLhs(), ss);
			ss << ")";

			if (term.getRhs()) {
				ss << op << "(";
				collectString(*term.getRhs(), ss);
				ss << ")";
			}
		}
	}

	std::string toString(const core::arithmetic::formula::Term& term) {
		std::stringstream ss;
		detail::collectString(term, ss);
		return ss.str();
	}
}
