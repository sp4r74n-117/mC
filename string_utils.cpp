#include "string_utils.h"

#include <sstream>

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> out;
	std::stringstream ss(s);
	std::string item;
	while(std::getline(ss, item, delim)) {
		out.push_back(item);
	}
	return out;
}

void replace_all(std::string& str, const std::string& substr, const std::string& replacement) {
	std::string::size_type n = 0;
	while((n = str.find(substr, n)) != std::string::npos) {
		str.replace(n, substr.size(), replacement);
		n += substr.size();
	}

}

bool starts_with(const std::string &s, const std::string &prefix) {
	return s.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string &s, const std::string &suffix) {
	return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}
