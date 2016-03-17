#pragma once

#include <iostream>
#include <sstream>

#include "ast.h"

class formatted_ostream {

	std::basic_ostream<char>& internal_stream;
	unsigned indent_level = 0;

public:
	formatted_ostream(std::basic_ostream<char>& internal_stream) : internal_stream(internal_stream) {}

	formatted_ostream& operator<<(const string& to_print);
};

template<typename T>
formatted_ostream& operator<<(formatted_ostream& stream, T&& to_print) {
	std::stringstream ss;
	ss << to_print;
	return stream.operator<<(ss.str());
}
