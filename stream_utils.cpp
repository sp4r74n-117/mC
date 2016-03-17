#include "stream_utils.h"

#include <algorithm>

#include "string_utils.h"

formatted_ostream& formatted_ostream::operator<<(const string& to_print) {
	auto lines = split(to_print, '\n');
	for(auto line: lines) {
		// remove indentation if "}"
		if(std::any_of(line.cbegin(), line.cend(), [](char c) { return '}' == c; })) indent_level--;
		for(size_t i=0; i<indent_level; ++i) internal_stream << "    ";
		// remove superfluous () on expression statements
		if(starts_with(line,"(") && ends_with(line,");")) line = line.substr(1, line.size()-3) + ";";
		internal_stream << line << std::endl;
		// add indentation if "{"
		if(std::any_of(line.cbegin(), line.cend(), [](char c) { return '{' == c; })) indent_level++;
	}
	return *this;
}
