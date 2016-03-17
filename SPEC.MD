mC Informal Specification
=========================

The mC language is a tiny subset of C. What this means is that a valid mC program (fragment) should be a valid C program (fragment), but generally not the other way around.


Grammar (EBNF)
--------------

	<type> ::= "int" | "float"

	<expression> ::= <binary_operation> | <single_expr>
	<binary_operation> ::= <single_expr> , <bin_op> , <expression>
	<single_expr> ::= <paren_expr> | <unary_expr> | <literal> | <variable>
	<binop> ::= "+" | "-" | "*" | "/" | "==" | "!=" | "<=" | ">=" | "<" | ">" | "="
	<paren_expr> ::= "(" , <expression> , ")"
	<unary_expr> ::= <unop> , <expression>
	<unop> ::= "-" | "!"
	<literal> ::= <int_lit> | <float_lit>
	<int_lit> ::= <digit> { <digit> }
	<float_lit ::= <digit> { <digit> } "." { <digit> }
	<digit> ::= 0 | 1 | 2 ... | 9
	<variable> ::= <identifier>
	<identfier> ::= <alpha> { <alnum> }
	<alpha> ::= 'a' | 'b' ... | 'z' | 'A' | 'B' ... | 'Z' | '_'
	<alnum> ::= <digit> | <alpha>

	<statement> ::= <if_stmt> | <decl_stmt> | <compound_stmt> | <expr_stmt>
	<if_stmt> ::= "if" , <paren_expr> , <statement> [ , "else" , <statement> ] , ";"
	<decl_stmt> ::= <type> , <identifier> [ , "=" , <expression> ] , ";"
	<compound_stmt> ::= "{" , { <statement> } , "}"
	<expr_stmt> ::= <expression> , ";"

Semantics
---------
Semantics are equivalent to the relevant subset of C, with some simplifications. In particular, this means that every new compound statement opens a new variable declaration scope.
