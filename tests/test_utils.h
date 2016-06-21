#pragma once

#define EXPECT(__ARG) if(!(__ARG)) { \
	std::cout << "TEST FAILED at line " << __LINE__ << std::endl; \
	std::cout << "     EXPRESSION: " << #__ARG << std::endl; \
}

#define EXPECT_MATCH(_PARSER, _STR, _CHECK) { \
	auto state = parser::parser_state(_STR.cbegin(), _STR.cend()); \
	auto result = _PARSER(state); \
	EXPECT(result); \
	EXPECT(state.s == _STR.cend()); \
	EXPECT(_CHECK(result)); \
}

#define EXPECT_MATCH_TYPED(_PARSER, _STR, _TYPE, _CHECK) { \
	auto state = parser::parser_state(_STR.cbegin(), _STR.cend()); \
	auto result = _PARSER(state); \
	EXPECT(result); \
	EXPECT(state.s == _STR.cend()); \
	EXPECT(typeid(*result) == typeid(_TYPE)); \
	EXPECT(_CHECK(std::dynamic_pointer_cast<_TYPE>(result))); }

#define EXPECT_NO_MATCH(_PARSER, _STR) { \
	auto state = parser::parser_state(_STR.cbegin(), _STR.cend()); \
	auto result = _PARSER(state); \
	EXPECT(!result); \
	EXPECT(state.s == _STR.cbegin()); }

#define EXPECT_ERROR(_PARSER, _STR) { \
	auto state = parser::parser_state(_STR.cbegin(), _STR.cend()); \
	try { auto result = _PARSER(state); \
	EXPECT("Expected exception, but none thrown"); } \
	catch(parser::parser_error&) {} }
