#pragma once
#include "basics.h"
#include <memory>

namespace utils {
	template<typename T>
	using Ptr = sptr<T>;

	template<typename T, typename U>
	Ptr<T> cast(const Ptr<U>& ptr) {
		return std::static_pointer_cast<T>(ptr);
	}

	template<typename T, typename U>
	Ptr<T> dyn_cast(const Ptr<U>& ptr) {
		return std::dynamic_pointer_cast<T>(ptr);
	}
}
