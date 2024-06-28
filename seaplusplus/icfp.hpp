#pragma once

#include <string>
#include <stdexcept>
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <limits>

typedef int64_t Integer;

inline Integer integer_from_token(std::string const &token) {
	assert(token.size() > 1 && token[0] == 'I');

	Integer val = 0;
	for (size_t i = 1; i < token.size(); ++i) {
		const char c = token[i];
		if (val > std::numeric_limits< Integer >::max() / 94) throw std::runtime_error("Potential integer overflow parsing token '" + token + "'.");
		val *= 94;

		static_assert('~' - '!' == 93, "Encoding space is the size we expect.");
		if (c >= '!' && c <= '~') {
			val += Integer(c - '!');
		} else {
			throw std::runtime_error("Invalid character [" + std::to_string(uint32_t(c)) + "] in integer.");
		}
	}

	return val;
}


inline std::string token_from_integer(Integer const &integer) {
	std::string token = "I";

	if (integer < 0) throw std::runtime_error("Cannot encode a negative integer in a single token.");
	if (integer == 0) token += '0';
	else {
		Integer val = integer;
		while (val > 0) {
			token += ('!' + char(val % 94));
			val /= 94;
		}
		std::reverse(token.begin() + 1, token.end());
	}
	return token;
}


//string encode/decode:

//index this with char - 33:
//                                  0         1         2         3         4         5         6         7         8         9
//                                  0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
[[maybe_unused]] static const char *DECODE_STRING = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";

inline std::string string_from_token(std::string const &token) {
	assert(!token.empty() && token[0] == 'S');

	std::string ret;
	ret.reserve(token.size()-1);
	for (size_t i = 1; i < token.size(); ++i) {
		const char c = token[i];
		if (c >= 33 && c <= 126) {
			ret += DECODE_STRING[c - 33];
		} else {
			throw std::runtime_error("Failed to decode [" + std::to_string(size_t(c)) + "].");
		}
	}

	return ret;
}

//index this with char:
[[maybe_unused]] static const char ENCODE_STRING[128] = "..........~.....................}_`abcdefghijklmUVWXYZ[\\]^nopqrst;<=>?@ABCDEFGHIJKLMNOPQRSTuvwxyz!\"#$%&'()*+,-./0123456789:.{.|";


inline std::string token_from_string(std::string const &string) {
	std::string token = "S";
	token.reserve(string.size() + 1);

	for (const char c : string) {
		if (size_t(c) + 1 >= sizeof(ENCODE_STRING)) throw std::runtime_error("Failed to encode [" + std::to_string(size_t(c)) + "].");
		token += ENCODE_STRING[size_t(c)];
	}

	assert(token.size() == string.size() + 1);
	return token;
}
