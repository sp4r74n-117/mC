#include "core/analysis/analysis.h"
#include <algorithm>

namespace core {
namespace analysis {
	bool isVariable(const ValuePtr& value) {
		return value && !isConstant(value);
	}

	bool isTemporary(const ValuePtr& value) {
		return isVariable(value) && value->getValueCategory() == Value::VC_Temporary;
	}

	bool isMemory(const ValuePtr& value) {
		return isVariable(value) && value->getValueCategory() == Value::VC_Memory;
	}

	bool isOffset(const ValuePtr& value) {
		return isVariable(value) && value->getValueCategory() == Value::VC_Temporary &&
			value->getValueType() == Value::VT_Memory;
	}

	bool isLValue(const ValuePtr& value) {
		return isMemory(value) || isOffset(value);
	}

	bool isConstant(const ValuePtr& value) {
		return value && value->getValueCategory() == Value::VC_Constant;
	}

	bool isIntConstant(const ValuePtr& value) {
		return isConstant(value) && value->getValueType() == Value::VT_IntConstant;
	}

	bool isFloatConstant(const ValuePtr& value) {
		return isConstant(value) && value->getValueType() == Value::VT_FloatConstant;
	}
}
}
