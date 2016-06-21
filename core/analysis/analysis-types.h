#pragma once
#include "core/core.h"

namespace core {
namespace analysis {
namespace types {
	bool isType(const NodePtr& type);
	bool isType(const TypePtr& type, Type::TypeId id);
	bool isInt(const TypePtr& type);
	bool isFloat(const TypePtr& type);
	bool isVoid(const TypePtr& type);
	bool isArray(const TypePtr& type);
	bool isFunction(const TypePtr& type);
	bool isCallable(const FunctionTypePtr& type, const ValueList& args);
	bool isCallable(const FunctionTypePtr& type, const TypeList& args);
	bool hasReturn(const FunctionTypePtr& type);
	FunctionTypePtr getFunctionType(const TypePtr& type);
	TypePtr getReturnType(const FunctionTypePtr& fun);
	TypePtr getElementType(const TypePtr& type);
	TypeList extractTypes(const ValueList& values);
}
}
}
