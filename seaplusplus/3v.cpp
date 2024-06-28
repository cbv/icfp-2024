
#include <iostream>
#include <unordered_map>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

typedef int64_t Integer;

struct Cell {
	Integer value = 0;
	char op = '\0';
	Cell() = default;
	Cell(Integer const &value_) : value(value_) { }
	Cell(std::string const &token) {
		assert(token.size() >= 1);
		if      (token == "<") op = '<';
		else if (token == ">") op = '>';
		else if (token == "^") op = '^';
		else if (token == "v") op = 'v';
		else if (token == "+") op = '+';
		else if (token == "-") op = '-';
		else if (token == "*") op = '*';
		else if (token == "/") op = '/';
		else if (token == "%") op = '%';
		else if (token == "@") op = '@';
		else if (token == "=") op = '=';
		else if (token == "#") op = '#';
		else if (token == "S") op = 'S';
		else if (token == "A") op = 'A';
		else if (token == "B") op = 'B';
		else if (
			(token.size() == 1 && '0' <= token[0] && token[0] <= '9')
			|| (token.size() == 2 && token[0] == '-' && '0' <= token[1] && token[1] <= '9')
			|| (token.size() == 2 && '0' <= token[0] && token[0] <= '9' && '0' <= token[1] && token[1] <= '9')
			|| (token.size() == 3 && token[0] == '_' && '0' <= token[1] && token[1] <= '9' && '0' <= token[2] && token[2] <= '9')) {
			op = '\0';
			value = std::stoi(token);
		} else {
			throw std::runtime_error("Invalid grid cell content: '" + token + "'.");
		}
	}
	bool operator==(Cell const &other) const {
		return (op == other.op && value == other.value);
	}
};

std::string to_string(Cell const &cell) {
	if (cell.op == '\0') {
		return std::to_string(cell.value);
	} else {
		assert(cell.value == 0);
		return std::string(&cell.op, 1);
	}
}

typedef std::unordered_map< glm::ivec2, Cell > Grid;

void dump(Grid const &grid) {
	if (grid.empty()) {
		std::cout << "Empty grid." << std::endl;
	}
	glm::ivec2 min = grid.begin()->first;
	glm::ivec2 max = grid.begin()->first;
	for (auto const &kv : grid) {
		min = glm::min(min, kv.first);
		max = glm::max(max, kv.first);
	}
	std::cout << "[" << min.x << ", " << max.x << "]x[" << min.y << ", " << max.y << "]:\n";
	for (int32_t y = min.y; y <= max.y; ++y) {
		for (int32_t x = min.x; x <= max.x; ++x) {
			auto f = grid.find(glm::ivec2(x,y));
			if (x != min.x) std::cout << ' ';
			if (f == grid.end()) {
				std::cout << '.';
			} else {
				std::cout << to_string(f->second);
			}
		}
		std::cout << '\n';
	}
	std::cout.flush();
}

int main(int argc, char **argv) {

	if (argc != 3) {
		std::cout << "Usage:\n3v <A> <B> < program.3d" << std::endl;
		return 1;
	}
	
	Integer A = std::stoi(argv[1]);
	Integer B = std::stoi(argv[2]);

	//----------------------------------
	//load initial grid:

	Grid grid;

	glm::ivec2 at(0,0);

	std::string tok;
	for (;;) {
		int c = std::cin.get();
		bool is_end = (c == decltype(std::cin)::traits_type::eof());
		bool is_wsp = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
		bool is_newline = (c == '\n');

		if (is_end || is_wsp || is_newline) {
			if (tok != "") {
				if (tok == ".") {
					//empty cell
				} else {
					//make a cell
					grid.emplace(at, Cell(tok));
				}
				//move to next cell:
				tok = "";
				at.x += 1;
			}
			if (is_newline) {
				//move to next row:
				at.x = 0;
				at.y += 1;
			}
		} else {
			tok += char(c);
		}

		if (is_end) break;
	}

	
	std::cout << "------ as loaded ------" << std::endl;
	dump(grid);

	std::vector< Grid > ticks;
	ticks.emplace_back(grid);
	for (auto &cell : ticks[0]) {
		if (cell.second.op == 'A') {
			cell.second.op = '\0';
			cell.second.value = A;
		}
		if (cell.second.op == 'B') {
			cell.second.op = '\0';
			cell.second.value = B;
		}
	}

	std::cout << "------ ticks[0] ------" << std::endl;
	dump(ticks[0]);
	for (;;) {
		Grid const &prev = ticks.back();
		Grid next = prev;
		Grid writes;

		uint32_t tt_dest = -1U;
		Grid tt_writes;

		//get value from previous grid (if there is a value):
		auto read = [&prev](glm::ivec2 const &at) -> std::optional< Cell > {
			auto f = prev.find(at);
			if (f == prev.end()) return std::nullopt;
			else return f->second;
		};

		//consume value (removes from next grid):
		auto consume = [&next](glm::ivec2 const &at) {
			auto f = next.find(at);
			if (f != next.end()) next.erase(f);
		};
		
		//write value (accumulates into write buffer for copy into next grid):
		auto write = [&writes, &prev](glm::ivec2 const &at, Cell const &val) {
			auto ret = writes.emplace(at, val);
			if (ret.second) {
				//was a fresh write
			} else {
				//something already wrote here, check for write conflict:
				if (ret.first->second != val) {
					throw std::runtime_error("Writing conflicting values.");
				}
			}
		};

		//write value and trigger time travel:
		auto tt_write = [&tt_dest, &tt_writes, &ticks](int32_t dt, glm::ivec2 const &at, Cell const &val) {

			//figure out where we are traveling to:
			if (dt <= 0) throw std::runtime_error("negative time travel");
			int32_t dest = int32_t(ticks.size()) - dt - 1;
			if (dest < 0) throw std::runtime_error("time travel into pre-history");
			assert(uint32_t(dest) < ticks.size());

			if (tt_dest == -1U) {
				tt_dest = dest;
			} else if (tt_dest != uint32_t(dest)) {
				throw std::runtime_error("inconsistent time travel");
			}

			auto ret = tt_writes.emplace(at, val);
			if (ret.second) {
				//was a fresh write
			} else {
				//something already wrote here, check for write conflict:
				if (ret.first->second != val) {
					throw std::runtime_error("writing conflicting values during time travel.");
				}
			}
		};


		bool reduced = false;

		for (auto const &[at, cell] : next) {
			auto arrow = [&](glm::ivec2 const &step) {
				if (std::optional< Cell > val = read(at - step)) {
					consume(at - step);
					write(at + step, *val);
					reduced = true;
				}
			};
			auto imath = [&](auto &&op) {
				std::optional< Cell > a = read(at + glm::ivec2(-1,0));
				std::optional< Cell > b = read(at + glm::ivec2(0,-1));
				if (a && b && a->op == '\0' && b->op == '\0') {
					consume(at + glm::ivec2(-1,0));
					consume(at + glm::ivec2(0,-1));
					Cell result(op(a->value,b->value));
					write(at + glm::ivec2(1,0), result);
					write(at + glm::ivec2(0,1), result);
					reduced = true;
				}
			};

			if      (cell.op == '>') arrow(glm::ivec2(1,0));
			else if (cell.op == '<') arrow(glm::ivec2(-1,0));
			else if (cell.op == 'v') arrow(glm::ivec2(0,1));
			else if (cell.op == '^') arrow(glm::ivec2(0,-1));
			else if (cell.op == '+') imath([](Integer const &a, Integer const &b) { return a + b; });
			else if (cell.op == '-') imath([](Integer const &a, Integer const &b) { return a - b; });
			else if (cell.op == '*') imath([](Integer const &a, Integer const &b) { return a * b; });
			else if (cell.op == '/') imath([](Integer const &a, Integer const &b) { return a / b; });
			else if (cell.op == '%') imath([](Integer const &a, Integer const &b) { return a % b; });
			else if (cell.op == '=' || cell.op == '#') {
				//equality and nonequality work for ints and operators:
				std::optional< Cell > a = read(at + glm::ivec2(-1,0));
				std::optional< Cell > b = read(at + glm::ivec2(0,-1));
				if (a && b) {
					if ((cell.op == '=') == (*a == *b)) {
						consume(at + glm::ivec2(-1,0));
						consume(at + glm::ivec2(0,-1));
						write(at + glm::ivec2(1,0), *a);
						write(at + glm::ivec2(0,1), *b);
						reduced = true;
					}
				}

			} else if (cell.op == '@') {
				std::optional< Cell > v = read(at + glm::ivec2(0,-1));
				std::optional< Cell > dx = read(at + glm::ivec2(-1,0));
				std::optional< Cell > dy = read(at + glm::ivec2(1,0));
				std::optional< Cell > dt = read(at + glm::ivec2(0,1));
				if (v && dx && dx->op == '\0' && dy && dy->op == '\0' && dt && dt->op == '\0') {
					tt_write(dt->value, at + glm::ivec2(-dx->value, -dy->value), *v);
					reduced = true;
				}
			} else if (cell.op == 'S') /* pass */;
			else if (cell.op == '\0') /* pass */;
			else {
				std::cerr << "TODO: implement '" << to_string(cell) << "'." << std::endl;
				return 1;
			}
		}

		std::optional< Cell > output;

		auto resolve_writes = [&next, &writes, &output]() {
			for (auto const &[at, cell] : writes) {
				auto ret = next.emplace(at, cell);
				if (!ret.second) {
					if (ret.first->second.op == 'S') {
						//writing an output value!
						if (!output) {
							output = cell;
						} else if (*output != cell) {
							throw std::runtime_error("Writing conflicting outputs.");
						}
					}
					ret.first->second = cell;
				}
				next[at] = cell;
			}
		};

		//resolve writes to current board (and thus output) *before* time travel:
		resolve_writes();

		if (output) {
			std::cout << "Output Written: " << to_string(*output) << std::endl;
			break;
		}

		//if there was no output, do time travel:
		if (tt_dest != -1U) {
			std::cout << "Time traveling to ticks[" << tt_dest << "]." << std::endl;
			next = std::move(ticks[tt_dest]);
			writes = std::move(tt_writes);
			ticks.erase(ticks.begin() + tt_dest, ticks.end());
			resolve_writes();
		}

		if (output) {
			std::cout << "Output Written (during time travel): " << to_string(*output) << std::endl;
			break;
		}

		ticks.emplace_back(std::move(next));
		std::cout << "------ ticks[" << ticks.size() - 1 << "] ------" << std::endl;
		dump(ticks.back());

		if (!reduced) {
			std::cout << "No operator can reduce." << std::endl;
			break;
		}

	}
	

	return 0;
}
