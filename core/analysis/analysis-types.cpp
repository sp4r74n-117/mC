#include "core/analysis/analysis-types.h"

namespace core {
namespace analysis {
namespace types {
	bool isType(const NodePtr& type) {
		return type && type->getNodeCategory() == Node::NC_Type;
	}

	bool isType(const TypePtr& type, Type::TypeId id) {
		return type && type->getTypeId() == id;
	}

	bool isInt(const TypePtr& type) {
		return type && type->isInt();
	}

	bool isFloat(const TypePtr& type) {
		return type && type->isFloat();
	}

	bool isVoid(const TypePtr& type) {
			return type && type->isVoid();
	}

	bool isFunction(const TypePtr& type) {
		return type && type->isFunction();
	}

	bool isArray(const TypePtr& type) {
		return type && type->isArray();
	}

	FunctionTypePtr getFunctionType(const TypePtr& type) {
		assert(isFunction(type) && "given type is not a function");
		return dyn_cast<FunctionType>(type);
	}

	bool isCallable(const FunctionTypePtr& type, const ValueList& args) {
		return isCallable(type, extractTypes(args));
	}

	bool isCallable(const FunctionTypePtr& type, const TypeList& args) {
		return type->getParameterTypes() == args;
	}

	bool hasReturn(const FunctionTypePtr& type) {
		return !isVoid(type->getReturnType());
	}

	TypeList extractTypes(const ValueList& values) {
		TypeList types;
		for (const auto& value : values)
			types.push_back(value->getType());
		return types;
	}

	TypePtr getReturnType(const FunctionTypePtr& fun) {
		return fun->getReturnType();
	}

	TypePtr getElementType(const TypePtr& type) {
		if (!isArray(type)) return type;
		// otherwise extract the inner type
		return cast<ArrayType>(type)->getElementType();
	}
}
}
}
