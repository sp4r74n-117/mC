#pragma once
#include "utils/utils-pointer.h"

#include <iostream>
#include <sstream>
#include <type_traits>

namespace utils {
	class Printable {
	public:
		virtual ~Printable() {}
		virtual std::ostream& printTo(std::ostream& stream) const { return stream; }
	};

	std::string toString(const Printable& printable);
	std::string toString(const Ptr<Printable>& printable);
	template<typename T>
	typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
	toString(const T& value) { return "N" + std::to_string(value); }
}
