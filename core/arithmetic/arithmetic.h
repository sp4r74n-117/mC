#pragma once
#include "core/core.h"
#include <type_traits>
#include <set>

namespace core {
namespace arithmetic {
	template<typename T>
	typename std::enable_if<std::is_arithmetic<T>::value, T>::type getValue(const ValuePtr& val) {
		switch (val->getValueType()) {
		case Value::VT_IntConstant: 	return cast<IntConstant>(val)->getValue();
		case Value::VT_FloatConstant:	return cast<FloatConstant>(val)->getValue();
		default:
				assert(false && "getValue called with variable");
		}
	}

	bool isEvaluable(const ValuePtr& lhs, const ValuePtr& rhs);
	ValuePtr evaluate(NodeManager& manager, AssignInsn::OpType op, const ValuePtr& lhs, const ValuePtr& rhs);

	optional<ValuePtr> tryGcd(NodeManager& manager, const ValueList& values);

namespace formula {
	class Term;
	typedef Ptr<Term> TermPtr;

	class Term {
	public:
		typedef AssignInsn::OpType OpType;
		OpType getOp() const { return op; }
		void setOp(OpType op) { this->op = op; }
		const TermPtr& getLhs() const { return lhs; }
		void setLhs(const TermPtr& lhs) { this->lhs = lhs; }
		const TermPtr& getRhs() const { return rhs; }
		void setRhs(const TermPtr& rhs) { this->rhs = rhs; }
		const ValuePtr& getValue() const { return value; }
		void setValue(const ValuePtr& value) { this->value = value; }
		bool isValue() const { return value != nullptr; }
	private:
		OpType op;
		TermPtr lhs;
		TermPtr rhs;
		ValuePtr value;
	};

	template<typename Lambda>
	void visitValues(const TermPtr& term, Lambda lambda) {
		if (!term) return;

		if (term->isValue()) {
			lambda(term->getValue());
		} else {
			visitValues(term->getLhs(), lambda);
			visitValues(term->getRhs(), lambda);
		}
	}

	template<typename Lambda>
	void collectValues(const TermPtr& term, ValueList& values, Lambda pred) {
		if (!term) return;

		if (term->isValue() && pred(term->getValue())) {
			values.push_back(term->getValue());
		} else {
			collectValues(term->getLhs(), values, pred);
			collectValues(term->getRhs(), values, pred);
		}
	}

	TermPtr makeTerm(const ValuePtr& value);
	TermPtr makeTerm(Term::OpType op, const TermPtr& lhs);
	TermPtr makeTerm(Term::OpType op, const TermPtr& lhs, const TermPtr& rhs);
	TermPtr simplify(NodeManager& manager, const TermPtr& term);

	bool isDiophantine(const TermPtr& term);
	optional<int> trySolveDiophantine(NodeManager& manager, const TermPtr& lhs, const ValuePtr& rhs);
}
}
}
namespace utils {
	std::string toString(const core::arithmetic::formula::Term& term);
}
