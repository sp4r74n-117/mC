#pragma once

namespace utils {
	template<typename T>
	class optional {
		bool present;
		T value;
	public:
		optional() :
			present(false)
		{ }
		optional(const T& value) :
			present(true), value(value)
		{ }
		optional(T&& value) :
			present(true), value(std::move(value))
		{ }
		optional(const optional& rhs) = default;
		optional(optional&& rhs) = default;
		optional& operator =(const optional& rhs) = default;
		optional& operator =(optional&& rhs) = default;
		operator bool() const { return present; }
		T& operator *() { return value; }
	};
}
