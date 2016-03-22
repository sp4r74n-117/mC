#include <iostream>
#include <fstream>
#include <sstream>

#include "ast.h"
#include "parser.h"
#include "stream_utils.h"
#include "tests/parser_tests.h"

int main(int argc, char** argv) {

	test_parser();

	if(argc!=2) {
		std::cout << "usage: mC [file]" << std::endl;
		return -1;
	}

	std::ifstream input{argv[1]};
	std::stringstream buffer;
	buffer << input.rdbuf();
	auto source = buffer.str();

	auto tree = parser::parse(source);

	formatted_ostream out(std::cout);
	out << "Parsed:\n" << tree << "\n";
}
