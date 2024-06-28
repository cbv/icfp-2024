#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

#include <cassert>

[[maybe_unused]] static const char ENCODE_STRING[128] = "..........~.....................}_`abcdefghijklmUVWXYZ[\\]^nopqrst;<=>?@ABCDEFGHIJKLMNOPQRSTuvwxyz!\"#$%&'()*+,-./0123456789:.{.|";

int main(int argc, char **argv) {

	if (argc != 2) {
		std::cerr <<
		"Usage:\n"
		"    encode <tokens>\n"
		"Converts tokens into icfp representation (encodes strings, ints, bools)."
		<< std::endl;
		return 1;
	}

	std::string infilepath = argv[1];

	std::ifstream infile(infilepath, std::ios::binary);

	enum {
		STATE_OUTER,
		STATE_STRING,
		STATE_STRING_ESCAPE,
	} state = STATE_OUTER;
	std::string token;

	auto emit = [&token]() {
		std::cout << token << std::endl;
	};

	for (char c; (c = infile.get()) != decltype(infile)::traits_type::eof(); ) {

		if (state == STATE_OUTER) {
			if (c == '"') {
				token = "S";
				state = STATE_STRING;
			}
		} else if (state == STATE_STRING) {
			if (c == '"') {
				emit();
				state = STATE_OUTER;
			} else if (c == '\\') {
				state = STATE_STRING_ESCAPE;
			} else if (size_t(c) < sizeof(ENCODE_STRING)) {
				token += ENCODE_STRING[size_t(c)];
			} else {
				std::cerr << "UNENCODABLE: [" << size_t(c) << "]" << std::endl;
			}
		} else if (state == STATE_STRING_ESCAPE) {
			if (c == '"' || c == '\\' || c == 'n') {
				token += ENCODE_STRING[size_t(c)];
			} else {
				std::cerr << "UNRECOGNIZED ESCAPEE: '\\" << c << "' [" << size_t(c) << "]" << std::endl;
			}
			state = STATE_STRING;
		}
		
		std::cout << c << std::endl;
	}

	return 0;
}
