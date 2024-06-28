#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <limits>
#include <vector>

#include <cassert>


//index this with char - 33:
//                                  0         1         2         3         4         5         6         7         8         9
//                                  0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
[[maybe_unused]] static const char *DECODE_STRING = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";

//index this with char:
[[maybe_unused]] static const char ENCODE_STRING[128] = "..........~.....................}_`abcdefghijklmUVWXYZ[\\]^nopqrst;<=>?@ABCDEFGHIJKLMNOPQRSTuvwxyz!\"#$%&'()*+,-./0123456789:.{.|";

int main(int argc, char **argv) {

	/*
	//helpful encoding-table-maker:
	char ENCODE_STRING[128];
	for (size_t i = 0; i < 128; ++i) {
		ENCODE_STRING[i] = '.';
	}
	for (size_t i = 33; i <= 126; ++i) {
		ENCODE_STRING[size_t(DECODE_STRING[i-33])] = char(i);
	}

	ENCODE_STRING[127] = '\0';

	std::cout << ENCODE_STRING << std::endl;
	*/

	if (argc != 2) {
		std::cerr <<
		"Usage:\n"
		"    pretty <program.icfp>\n"
		"Parses and pretty-prints an icfp expression."
		<< std::endl;
		return 1;
	}

	std::string infilepath = argv[1];

	std::ifstream infile(infilepath, std::ios::binary);

	std::vector< size_t > indents; //count is remaining tokens to print at this indentation level

	auto print = [&indents](std::string const &text) {
		if (!indents.empty()) {
			while (!indents.empty() && indents.back() == 0) indents.pop_back();
			for (size_t i = 0; i < indents.size(); ++i) {
				std::cout << "    "; //shudder
			}
			if (!indents.empty()) indents.back() -= 1;
		}
		std::cout << text << std::endl;
	};

	auto error = [](std::string const &error) {
		std::cout << error << std::endl;
	};

	std::string tok;
	while (infile >> tok) {
		assert(tok.size() >= 1);

		if (tok[0] == 'T') {
			print("true");
		} else if (tok[0] == 'F') {
			print("false");
		} else if (tok[0] == 'I') {
			if (tok.size() < 1) {
				error("Integer with empty body.");
			} else {
				uint64_t val = 0;
				for (char c : tok.substr(1)) {
					if (val > std::numeric_limits< uint64_t >::max() / 94) {
						error("INTEGER OVERFLOW parsing integer value.");
					}
					val *= 94;

					static_assert('~' - '!' == 93, "Encoding space is the size we expect.");
					if (c >= '!' && c <= '~') {
						val += uint64_t(c - '!');
					} else {
						error("Uninterpretable character [" + std::to_string(uint32_t(c)) + "] in integer.");
					}
				}
				print(std::to_string(val));
			}
		} else if (tok[0] == 'S') {
			std::string translated;
			for (char c : tok.substr(1)) {
				if (c >= 33 && c <= 126) {
					translated += DECODE_STRING[c - 33];
				} else {
					translated += "[" + std::to_string(uint32_t(c)) + "]";
				}
			}
			print('"' + translated + '"');
		} else if (tok[0] == 'U') {
			if (tok.size() != 2) {
				error("Unary operator not one character long.");
			} else if (tok[1] == '-') { //integer negation
				print(tok);
			} else if (tok[1] == '!') { //boolean not
				print(tok);
			} else if (tok[1] == '#') { //string-to-int
				print(tok);
			} else if (tok[1] == '$') { //int-to-string
				print(tok);
			} else {
				error("Unrecognized unary operator.");
			}
			indents.emplace_back(1);
		} else if (tok[0] == 'B') {
			if (tok.size() != 2) {
				error("Binary operator not one character long.");
			} else if (tok[1] == '+') {
				print(tok);
			} else if (tok[1] == '-') {
				print(tok);
			} else if (tok[1] == '*') {
				print(tok);
			} else if (tok[1] == '/') {
				print(tok);
			} else if (tok[1] == '%') {
				print(tok);
			} else if (tok[1] == '<') {
				print(tok);
			} else if (tok[1] == '>') {
				print(tok);
			} else if (tok[1] == '=') {
				print(tok);
			} else if (tok[1] == '|') {
				print(tok);
			} else if (tok[1] == '&') {
				print(tok);
			} else if (tok[1] == '.') {
				print(tok);
			} else if (tok[1] == 'T') {
				print(tok);
			} else if (tok[1] == 'D') {
				print(tok);
			} else if (tok[1] == '$') {
				print(tok);
			} else {
				error("Unrecognized binary operator.");
			}
			indents.emplace_back(2);
		} else if (tok[0] == '?') {
			if (tok.size() != 1) {
				error("If with non-empty body.");
			} else {
				print(tok);
			}
			indents.emplace_back(3);
		} else if (tok[0] == 'v') {
			//variable
			if (tok.size() < 2) {
				error("variable without number.");
			} else {
				print(tok);
			}
		} else if (tok[0] == 'L') {
			//lambda abstraction
			if (tok.size() < 2) {
				error("Lambda without variable number.");
			} else {
				print(tok);
			}
			indents.emplace_back(2);
		} else {
			print("[" + tok.substr(0,1) + "] unrecognized \"" + tok + "\"");
		}

	}

	return 0;
}
