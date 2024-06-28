#include "icfp.hpp"

#include <iostream>
#include <string>
#include <cstdint>
#include <cassert>

int main(int, char **) {

	enum {
		STATE_OUTER,
		STATE_STRING,
		STATE_STRING_ESCAPE,
		STATE_INTEGER,
	} state = STATE_OUTER;

	Integer integer;
	std::string string;

	bool prev_token = false;
	auto emit = [&prev_token](std::string const &token) {
		if (prev_token) std::cout << ' ';
		prev_token = true;
		std::cout << token;
		std::cout.flush();
	};

	while (true) {
		const char c = std::cin.get();
		const bool is_eof = (c == decltype(std::cin)::traits_type::eof());
		const bool is_wsp = (c == ' ' || c == '\n' || c == '\r' || c == '\t');

		if (state == STATE_OUTER) {
			if (c == '"') {
				string = "";
				state = STATE_STRING;
			} else if (c >= '0' && c <= '9') {
				integer = Integer(c - '0');
				state = STATE_INTEGER;
			} else if (is_eof || is_wsp) {
				//ignore
			} else {
				std::cerr << "CONFUSED by: '" << c << "' [" << size_t(c) << "]" << std::endl;
			}
		} else if (state == STATE_STRING) {
			if (c == '"') {
				emit(token_from_string(string));
				string = "";
				state = STATE_OUTER;
			} else if (c == '\\') {
				state = STATE_STRING_ESCAPE;
			} else if (size_t(c) < sizeof(ENCODE_STRING)) {
				string += c;
			} else {
				std::cerr << "UNENCODABLE: [" << size_t(c) << "]" << std::endl;
			}
		} else if (state == STATE_STRING_ESCAPE) {
			if (c == '"' || c == '\\') {
				string += c;
			} else if (c == 'n') {
				string += '\n';
			} else {
				std::cerr << "UNRECOGNIZED ESCAPEE: '\\" << c << "' [" << size_t(c) << "]" << std::endl;
			}
			state = STATE_STRING;
		} else if (state == STATE_INTEGER) {
			if (c >= '0' && c <= '9') {
				integer *= 10;
				integer += Integer(c - '0');
			} else if (is_wsp || is_eof) {
				emit(token_from_integer(integer));
				state = STATE_OUTER;
			} else {
				std::cerr << "UNRECOGNIZED DIGIT: '" << c << "' [" << size_t(c) << "]" << std::endl;
			}
		}

		if (is_eof) break;
	}

	if (state != STATE_OUTER) {
		std::cerr << "Ended in medas res!" << std::endl;
	}

	if (prev_token) std::cout << std::endl;

	return 0;
}
