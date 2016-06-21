#pragma once
#include "core/core.h"
#include <unordered_map>
#include <type_traits>
#include <set>

namespace core {
namespace analysis {
	bool isVariable(const ValuePtr& value);
	bool isTemporary(const ValuePtr& value);
	bool isMemory(const ValuePtr& value);
	bool isOffset(const ValuePtr& value);
	bool isLValue(const ValuePtr& value);
	bool isConstant(const ValuePtr& value);
	bool isIntConstant(const ValuePtr& value);
	bool isFloatConstant(const ValuePtr& value);
}
}
