#include "icfp.hpp"

#include <vector>
#include <iostream>
#include <iterator>
#include <unordered_map>

bool is_integer(std::vector< std::string > const &tokens, Integer *value = nullptr) {
	if (tokens.size() == 1 && tokens[0].size() >= 2 && tokens[0][0] == 'I') {
		//I...
		if (value) *value = integer_from_token(tokens[0]);
		return true;
	} else  if (tokens.size() == 2 && tokens[0] == "U-" && tokens[1].size() >= 2 && tokens[1][0] == 'I') {
		//U- I...
		if (value) *value = -integer_from_token(tokens[1]);
		return true;
	} else {
		return false;
	}
}

template< typename OutIterator >
void push_integer(OutIterator &out, Integer const &integer) {
	if (integer < 0) {
		*out = "U-";
		++out;
		*out = token_from_integer(-integer);
		++out;
	} else {
		*out = token_from_integer(integer);
		++out;
	}
}

//NOTE: could actually be slightly wrong with token stream *starts with* a lambda expression but is larger
bool is_lambda(std::vector< std::string > const &tokens, std::string *variable = nullptr) {
	if (tokens.size() >= 1 && tokens[0].size() >= 2 && tokens[0][0] == 'L') {
		if (variable) *variable = tokens[0].substr(1);
		return true;
	} else {
		return false;
	}
}

//copies the next expression from the token stream (if such an expression exists):
template< typename InIterator, typename OutIterator>
bool copy(InIterator &next, InIterator const &end, OutIterator &&out) {
	if (next == end) return false;
	std::string token = *next; ++next;
	if (token[0] == 'S' || token[0] == 'I' || token[0] == 'T' || token[0] == 'F' || token[0] == 'v') {
		//it's a single token:
		*out = token; ++out;
		return true;
	} else if (token[0] == 'L' || token[0] == 'U') {
		//it's a token that takes one argument:
		*out = token; ++out;
		return ::copy(next, end, out);
	} else if (token[0] == 'B') {
		//it's a token that takes two arguments:
		*out = token; ++out;
		return ::copy(next, end, out)
		    && ::copy(next, end, out);
	} else if (token[0] == '?') {
		//it's a token that takes three arguments:
		*out = token; ++out;
		return ::copy(next, end, out)
		    && ::copy(next, end, out)
		    && ::copy(next, end, out);
	} else {
		std::cerr << "Don't know how to copy: '" << token << "'" << std::endl;
		return false;
	}
}

struct Binding;
typedef std::unordered_map< std::string, Binding > Context;

struct Binding {
	std::vector< std::string > expr;
	Context context;
};


template< typename InIterator, typename OutIterator>
void eval(InIterator &next, InIterator const &end, OutIterator &&out, Context const &with) {
	assert(next != end);
	std::string op = *next; ++next;

	//DEBUG:
	std::cout << "eval " << op << " with {";
	for (auto const &kv : with) {
		std::cout << kv.first << ":[";
		for (auto const &tok : kv.second.expr) {
			std::cout << ' ' << tok;
		}
		std::cout << " ]";
	}
	std::cout << "}" << std::endl;


	assert(!op.empty());
	if (op[0] == 'S' || op[0] == 'I' || op[0] == 'T' || op[0] == 'F') {
		//it's a value!
		*out = op;
		++out;
	} else if (op[0] == 'v') {
		//it's a variable!
		std::string variable = op.substr(1);
		auto f = with.find(variable);
		if (f != with.end()) {
			Binding binding = f->second;
			auto expr_next = binding.expr.begin();
			eval(expr_next, binding.expr.end(), out, binding.context);
		} else {
			std::cout << "Free: " << op << std::endl; //DEBUG
			//free variable, just copy it:
			*out = op;
			++out;
		}
	} else if (op[0] == 'L') {
		//it's a (deferred, in some sense) value!
		*out = op;
		++out;
		::copy(next, end, out);
	} else if (op[0] == 'U') {
		if (op.size() != 2) throw std::runtime_error("Unary op without operation.");
		//do unary op:
		std::vector< std::string > arg0;
		eval(next, end, std::back_inserter(arg0), with);

		Integer integer;
		if (op[1] == '-' && is_integer(arg0, &integer)) {
			integer = -integer;
			push_integer(out, integer);
		//TODO: other unary expressions!
		} else {
			std::cerr << "Wedged unary expression." << std::endl;
			*out = op;
			++out;
			std::copy(arg0.begin(), arg0.end(), out);
		}
	} else if (op[0] == 'B') {
		if (op.size() != 2) throw std::runtime_error("Binary op without operation.");


		std::cout << "withAAAAAAA: { ";
	for (auto const &kv : with) {
		std::cout << kv.first << ":[";
		for (auto const &tok : kv.second.expr) {
			std::cout << ' ' << tok;
		}
		std::cout << " ]";
	}
	std::cout << " }" << std::endl;


		//do binary op:
		std::vector< std::string > arg0;
		eval(next, end, std::back_inserter(arg0), with);
		std::vector< std::string > arg1;
		if (op[1] == '$') {
			//expressions in application are lazily evaluated
			::copy(next, end, std::back_inserter(arg1));
		} else {
			eval(next, end, std::back_inserter(arg1), with);
		}

		//DEBUG:
		std::cout << op << " [";
		for (auto const &tok : arg0) std::cout << ' ' << tok;
		std::cout << " ] [";
		for (auto const &tok : arg1) std::cout << ' ' << tok;
		std::cout << " ]" << std::endl;

		std::string variable;
		if (op[1] == '$' && is_lambda(arg0, &variable)) {
			auto new_with = with;

				std::cout << "with: { ";
	for (auto const &kv : with) {
		std::cout << kv.first << ":[";
		for (auto const &tok : kv.second.expr) {
			std::cout << ' ' << tok;
		}
		std::cout << " ]";
	}
	std::cout << " }" << std::endl;


		std::cout << "new_with: { ";
	for (auto const &kv : new_with) {
		std::cout << kv.first << ":[";
		for (auto const &tok : kv.second.expr) {
			std::cout << ' ' << tok;
		}
		std::cout << " ]";
	}
	std::cout << " }" << std::endl;

			new_with[variable] = Binding{.expr = arg1, .context = with};

				std::cout << "new_with: { ";
	for (auto const &kv : new_with) {
		std::cout << kv.first << ":[";
		for (auto const &tok : kv.second.expr) {
			std::cout << ' ' << tok;
		}
		std::cout << " ]";
	}
	std::cout << " }" << std::endl;



			auto arg0_next = arg0.begin();
			++arg0_next;
			eval(arg0_next, arg0.end(), out, new_with);
		} else {
			std::cerr << "Wedged binary expression." << std::endl;
			*out = op;
			++out;
			std::copy(arg0.begin(), arg0.end(), out);
			std::copy(arg1.begin(), arg1.end(), out);
		}

	} else if (op[0] == '?') {
		if (op.size() != 1) throw std::runtime_error("Ternary op with trailing junk.");
		//do ternary op:

		std::vector< std::string > arg0;
		eval(next, end, std::back_inserter(arg0), with);
		std::vector< std::string > arg1;
		eval(next, end, std::back_inserter(arg1), with);
		std::vector< std::string > arg2;
		eval(next, end, std::back_inserter(arg2), with);

		//TODO: actually evaluate argument and such

		{
			std::cerr << "Wedged ternary expression." << std::endl;
			*out = op;
			++out;
			std::copy(arg0.begin(), arg0.end(), out);
			std::copy(arg1.begin(), arg1.end(), out);
			std::copy(arg2.begin(), arg2.end(), out);
		}
		
	} else {
		std::cerr << "Unimplemented op: '" << op << "'" << std::endl;
	}
}

int main(int, char **) {
	std::vector< std::string > tokens;

	{ //read initial list of tokens:
		std::string token;
		while (std::cin >> token) {
			tokens.emplace_back(token);
		}
	}

	std::vector< std::string > result;
	auto next = tokens.begin();
	while (next != tokens.end()) {
		Context with;
		eval(next, tokens.end(), std::back_inserter(result), with);
	}

	for (auto const &token : result) {
		if (&token != &result[0]) std::cout << ' ';
		std::cout << token;
	}
	std::cout << std::endl;

	return 0;
}
