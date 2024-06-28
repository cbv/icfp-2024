#include "icfp.hpp"

#include <memory>
#include <unordered_map>
#include <iostream>
#include <stdexcept>

struct Expr {
	std::string token;
	Expr const *arg0 = nullptr;
	Expr const *arg1 = nullptr;
	Expr const *arg2 = nullptr;
};

std::string to_string(Expr const &expr) {
	std::string ret =expr.token;
	if (expr.arg0) { ret += ' '; ret += to_string(*expr.arg0); }
	if (expr.arg1) { ret += ' '; ret += to_string(*expr.arg1); }
	if (expr.arg2) { ret += ' '; ret += to_string(*expr.arg2); }
	return ret;
}


Expr const *parse(std::istream *from_) {
	assert(from_);
	auto &from = *from_;

	std::string token;
	if (!(from >> token)) throw std::runtime_error("Ran out of input while parsing expression.");
	assert(!token.empty());

	if (token[0] == 'I' || token[0] == 'S' || token[0] == 'T' || token[0] == 'F' || token[0] == 'v') {
		//value! (or variable)
		return new Expr{.token = token};
	} else if (token[0] == 'U' || token[0] == 'L') {
		//unary op!
		return new Expr{
			.token = token,
			.arg0 = parse(&from)
		};
	} else if (token[0] == 'B') {
		//binary op!
		return new Expr{
			.token = token,
			.arg0 = parse(&from),
			.arg1 = parse(&from)
		};
	} else if (token[0] == '?') {
		//ternary op!
		return new Expr{
			.token = token,
			.arg0 = parse(&from),
			.arg1 = parse(&from),
			.arg2 = parse(&from)
		};
	} else {
		throw std::runtime_error("Unrecognized token: '" + token + "'");
	}
}

Integer integer_from_expr(Expr const &expr) {
	if (expr.token[0] == 'I') {
		return integer_from_token(expr.token);
	} else if (expr.token == "U-" && expr.arg0 && expr.arg0->token[0] == 'I') {
		return -integer_from_token(expr.arg0->token);
	} else {
		throw std::runtime_error("Expected integer expression.");
	}
}

Expr const *expr_from_integer(Integer const &integer) {
	if (integer < 0) {
		return new Expr{
			.token = "U-",
			.arg0 = new Expr{ .token = token_from_integer(-integer) }
		};
	} else {
		return new Expr{ .token = token_from_integer(integer) };
	}
}

std::string string_from_expr(Expr const &expr) {
	if (expr.token[0] == 'S') {
		return string_from_token(expr.token);
	} else {
		throw std::runtime_error("Expected string value.");
	}
}

Expr const *expr_from_string(std::string const &string) {
	return new Expr{ .token = token_from_string(string) };
}

bool boolean_from_expr(Expr const &expr) {
	if (expr.token[0] == 'T') {
		return true;
	} else if (expr.token[0] == 'F') {
		return false;
	} else {
		throw std::runtime_error("Expected boolean value, got [" + to_string(expr) + "].");
	}
}

Expr const *expr_from_boolean(bool const boolean) {
	static const Expr *true_expr = new Expr{.token = "T"};
	static const Expr *false_expr = new Expr{.token = "F"};
	return (boolean ? true_expr : false_expr);
}

Expr const *subst(Expr const *expr, std::string const &variable, Expr const *binding) {
	if (expr == nullptr) return expr;

	//NOTE: should probably mark up free variables or something to avoid capture here.
	if (expr->token[0] == 'v' && expr->token.substr(1) == variable) return binding;

	if (expr->token[0] == 'L' && expr->token.substr(1) == variable) {
		//variable shadowed! nice.
		return expr;
	}

	Expr const *new_arg0 = subst(expr->arg0, variable, binding);
	Expr const *new_arg1 = subst(expr->arg1, variable, binding);
	Expr const *new_arg2 = subst(expr->arg2, variable, binding);

	if (new_arg0 == expr->arg0
	 && new_arg1 == expr->arg1
	 && new_arg2 == expr->arg2) {
		return expr;
	}

	return new Expr{
		.token = expr->token,
		.arg0 = new_arg0,
		.arg1 = new_arg1,
		.arg2 = new_arg2
	};
}

Expr const *eval(Expr const &expr) {
	assert(expr.token.size() >= 1);

	if (expr.token[0] == 'S' || expr.token[0] == 'I' || expr.token[0] == 'T' || expr.token[0] == 'F') {
		return &expr;
	} else if (expr.token[0] == 'v') {
		//if we got here it's a free variable
		return &expr;
	} else if (expr.token[0] == 'L') {
		//if we got here it's a lambda waiting on an application
		return &expr;
	} else if (expr.token[0] == 'U') {
		assert(expr.token.size() == 2);
		assert(expr.arg0);
		Expr const *arg0 = eval(*expr.arg0);
		assert(arg0);

		if (expr.token[1] == '-') { //integer negation
			Integer integer = integer_from_expr(*arg0);
			integer = -integer;
			return expr_from_integer(integer);
		} else if (expr.token[1] == '!') { //boolean negation
			bool boolean = boolean_from_expr(*arg0);
			return expr_from_boolean(!boolean);
		} else if (expr.token[1] == '#') { //string-to-int
			std::string val = arg0->token;
			if (val[0] != 'S') throw std::runtime_error("No string to coerce.");
			val[0] = 'I';
			return new Expr{.token = val};
		} else if (expr.token[1] == '$') { //int-to-string
			std::string val = arg0->token;
			if (val[0] != 'I') throw std::runtime_error("No int (or negative int?) to coerce.");
			val[0] = 'S';
			return new Expr{.token = val};
		} else {
			throw std::runtime_error("Unsupported unop '" + expr.token + "'");
		}
	} else if (expr.token[0] == 'B') {
		assert(expr.token.size() == 2);
		assert(expr.arg0);
		assert(expr.arg1);

		Expr const *arg0 = eval(*expr.arg0);
		if (expr.token[1] == '$') {
			//special case for application:

			if (arg0->token[0] != 'L') throw std::runtime_error("trying to apply a non-lambda [" + to_string(*arg0) + "].");
			if (!arg0->arg0) throw std::runtime_error("trying to apply a lambda with no subexpression.");

			Expr const *reduced = subst(arg0->arg0, arg0->token.substr(1), expr.arg1);
			assert(reduced);
			return eval(*reduced);
		}

		//non-special case for other binops:
		Expr const *arg1 = eval(*expr.arg1);

		if (expr.token[1] == '+') {
			Integer val0 = integer_from_expr(*arg0);
			Integer val1 = integer_from_expr(*arg1);
			return expr_from_integer(val0 + val1);
		} else if (expr.token[1] == '-') {
			Integer val0 = integer_from_expr(*arg0);
			Integer val1 = integer_from_expr(*arg1);
			return expr_from_integer(val0 - val1);
		} else if (expr.token[1] == '*') {
			Integer val0 = integer_from_expr(*arg0);
			Integer val1 = integer_from_expr(*arg1);
			return expr_from_integer(val0 * val1);
		} else if (expr.token[1] == '/') {
			Integer val0 = integer_from_expr(*arg0);
			Integer val1 = integer_from_expr(*arg1);
			return expr_from_integer(val0 / val1);
		} else if (expr.token[1] == '%') {
			Integer val0 = integer_from_expr(*arg0);
			Integer val1 = integer_from_expr(*arg1);
			return expr_from_integer(val0 % val1);
		} else if (expr.token[1] == '>') {
			Integer val0 = integer_from_expr(*arg0);
			Integer val1 = integer_from_expr(*arg1);
			return expr_from_boolean(val0 > val1);
		} else if (expr.token[1] == '<') {
			Integer val0 = integer_from_expr(*arg0);
			Integer val1 = integer_from_expr(*arg1);
			return expr_from_boolean(val0 < val1);
		} else if (expr.token[1] == '=') {
			if ((arg0->token[0] == 'I' || arg0->token == "U-") && (arg1->token[0] == 'I' || arg1->token == "U-")) {
				Integer val0 = integer_from_expr(*arg0);
				Integer val1 = integer_from_expr(*arg1);
				return expr_from_boolean(val0 == val1);
			} else if (arg0->token[0] == 'S' && arg1->token[0] == 'S') {
				std::string val0 = string_from_expr(*arg0);
				std::string val1 = string_from_expr(*arg1);
				return expr_from_boolean(val0 == val1);
			} else if ( (arg0->token[0] == 'T' || arg0->token[0] == 'F') && (arg0->token[0] == 'T' || arg0->token[0] == 'F') ) {
				bool val0 = boolean_from_expr(*arg0);
				bool val1 = boolean_from_expr(*arg1);
				return expr_from_boolean(val0 == val1);
			} else {
				return expr_from_boolean(false);
			}
		} else if (expr.token[1] == '|') {
			bool val0 = boolean_from_expr(*arg0);
			bool val1 = boolean_from_expr(*arg1);
			return expr_from_boolean(val0 || val1);
		} else if (expr.token[1] == '&') {
			bool val0 = boolean_from_expr(*arg0);
			bool val1 = boolean_from_expr(*arg1);
			return expr_from_boolean(val0 && val1);
		} else if (expr.token[1] == '.') {
			std::string val0 = string_from_expr(*arg0);
			std::string val1 = string_from_expr(*arg1);
			return expr_from_string(val0 + val1);
		} else if (expr.token[1] == 'T') {
			Integer val0 = integer_from_expr(*arg0);
			std::string val1 = string_from_expr(*arg1);
			return expr_from_string(val1.substr(0, val0));
		} else if (expr.token[1] == 'D') {
			Integer val0 = integer_from_expr(*arg0);
			std::string val1 = string_from_expr(*arg1);
			return expr_from_string(val1.substr(val0));
		} else {
			throw std::runtime_error("Unsupported binop '" + expr.token + "'");
		}

	} else if (expr.token[0] == '?') {
		assert(expr.arg0);
		Expr const *arg0 = eval(*expr.arg0);

		bool boolean = boolean_from_expr(*arg0);
		if (boolean) {
			return eval(*expr.arg1);
		} else {
			return eval(*expr.arg2);
		}
	} else {
		throw std::runtime_error("What is token '" + expr.token + "'?");
	}
}

int main(int, char **) {

	const Expr *expr = parse(&std::cin);

	std::string token;
	if (std::cin >> token) {
		std::cerr << "WARNING: Trailing junk: " << token << std::endl;
	}

	const Expr *result = eval(*expr);

	std::cout << to_string(*result) << std::endl;

	return 0;
}
