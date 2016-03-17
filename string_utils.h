#pragma once

#include <string>
#include <vector>

std::vector<std::string> split(const std::string &s, char delim);
void replace_all(std::string& str, const std::string& substr, const std::string& replacement);

bool starts_with(const std::string &s, const std::string &prefix);
bool ends_with(const std::string &s, const std::string &suffix);
