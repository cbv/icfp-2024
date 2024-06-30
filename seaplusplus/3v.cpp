
#include <iostream>
#include <unordered_map>
#include <optional>
#include "string.h"

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <gmpxx.h>

typedef mpz_class Integer;

struct Cell {
	Integer value = Integer(0);
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
		else if (token.size() >= 1 && (token[0] == '-' || ('0' <= token[0] && token[0] <= '9'))) {
			for (size_t i = 1; i < token.size(); ++i) {
				if (!('0' <= token[i] && token[i] <= '9')) {
					throw std::runtime_error("alphabetic character in numerical constant '" + token + "'.");
				}
			}
			op = '\0';
			value = Integer(token);
			if (value < -99 || value > 99) {
				std::cerr << "WARNING: allowing out-of-range numeric literal '" << token << "'." << std::endl;
			}
		} else {
			throw std::runtime_error("Invalid grid cell content: '" + token + "'.");
		}
	}
	bool operator==(Cell const &other) const {
		return (op == other.op && value == other.value);
	}
};

bool TRUNCATE_LONG_VALUES = true;

std::string to_string(Cell const &cell) {
	if (cell.op == '\0') {
		std::string str = cell.value.get_str();
		if (TRUNCATE_LONG_VALUES && str.size() > 10) {
			str = str.substr(0,3) + ".." + str.substr(str.size()-3);
		}
		return str;
	} else {
		assert(cell.value == 0);
		return std::string(&cell.op, 1);
	}
}

typedef std::unordered_map< glm::ivec2, Cell > Grid;

void dump(Grid const &grid, bool json, int num_ticks) {
	if (grid.empty() && !json) {
		std::cout << "Empty grid." << std::endl;
	}
	glm::ivec2 min = grid.begin()->first;
	glm::ivec2 max = grid.begin()->first;
	for (auto const &kv : grid) {
		min = glm::min(min, kv.first);
		max = glm::max(max, kv.first);
	}

   if (json) {
     std::cout << "{\"t\":\"frame\",";
     std::cout << "\"time\":" << num_ticks << ",";
     std::cout << "\"min\":[" << min.x << "," << min.y << "],\"max\":[" << max.x << "," << max.y << "],";
   }
   else {
     std::cout << "[" << min.x << ", " << max.x << "]x[" << min.y << ", " << max.y << "]:\n";
   }

	std::vector< uint32_t > widths(max.x - min.x + 1, 2);
	for (auto const &[at, cell] : grid) {
		widths[at.x - min.x] = std::max< uint32_t >(widths[at.x - min.x], to_string(cell).size());
	}

   if (json) {
     std::cout << "\"frame\":\"";
   }


	for (int32_t y = min.y; y <= max.y; ++y) {
		for (int32_t x = min.x; x <= max.x; ++x) {
			auto f = grid.find(glm::ivec2(x,y));
			if (x != min.x) std::cout << ' ';
			std::string str;
			if (f == grid.end()) {
				str = ".";
			} else {
				str = to_string(f->second);
			}
			while (str.size() < widths[x-min.x]) str = ' ' + str;
			std::cout << str;
		}
      if (json) {
        std::cout << "\\n";
      }
      else {
        std::cout << '\n';
      }
	}

   if (json) {
     std::cout << "\"}";
   }

	std::cout.flush();
}

int main(int argc, char **argv) {

   bool json = false;
   int cur_arg = 1;
   int limit_steps = -1;

   for (;;) {
     if (!strcmp(argv[cur_arg], "--json")) {
       json = true;
       cur_arg++;
       continue;
     }
     if (!strcmp(argv[cur_arg], "--limit")) {
       limit_steps = std::stoi(argv[cur_arg+1]);
       cur_arg += 2;
       continue;
     }
     break;
   }

   if (argc - cur_arg != 2) {
     std::cout << "Usage:\n3v [--json] [--limit <LIMIT STEPS>] <A> <B> < program.3d" << std::endl;
     return 1;
   }

	Integer A = Integer(std::string(argv[cur_arg]));
	Integer B = Integer(std::string(argv[cur_arg+1]));

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
					try {
						grid.emplace(at, Cell(tok));
					} catch (std::runtime_error &e) {
						std::cerr << "Ignoring cell " << at.x << ", " << at.y << ": " << e.what() << std::endl;
					}
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

   if (json) {
     std::cout << "[" << std::endl;
   }
   else {
     std::cout << "------ as loaded ------" << std::endl;
   }
	dump(grid, json, 0);

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

   if (json) {
     std::cout << "," << std::endl;
   }
   else {
     std::cout << "------ ticks[0] ------" << std::endl;
   }
	dump(ticks[0], json, 1);

   int sim_steps = 0;
   try {
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
			else if (cell.op == '/') imath([](Integer const &a, Integer const &b) {
           if (b == 0)
             throw std::runtime_error("divide by zero");
           return a / b;
         });
			else if (cell.op == '%') imath([](Integer const &a, Integer const &b) {
           if (b == 0)
             throw std::runtime_error("divide by zero");
           return a % b;
         });
			else if (cell.op == '=' || cell.op == '#') {
				//equality and nonequality work for ints and operators:
				std::optional< Cell > a = read(at + glm::ivec2(-1,0));
				std::optional< Cell > b = read(at + glm::ivec2(0,-1));
				if (a && b) {
					if ((cell.op == '=') == (*a == *b)) {
						consume(at + glm::ivec2(-1,0));
						consume(at + glm::ivec2(0,-1));
						write(at + glm::ivec2(0,1), *a);
						write(at + glm::ivec2(1,0), *b);
						reduced = true;
					}
				}

			} else if (cell.op == '@') {
				std::optional< Cell > v = read(at + glm::ivec2(0,-1));
				std::optional< Cell > dx = read(at + glm::ivec2(-1,0));
				std::optional< Cell > dy = read(at + glm::ivec2(1,0));
				std::optional< Cell > dt = read(at + glm::ivec2(0,1));
				if (v && dx && dx->op == '\0' && dy && dy->op == '\0' && dt && dt->op == '\0') {
					tt_write(dt->value.get_si(), at + glm::ivec2(-dx->value.get_si(), -dy->value.get_si()), *v);
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
        if (json) {
          std::cout << ",\n{\"t\":\"output\",\"output\":" << to_string(*output) << "}";
        }
        else {
          std::cout << "Output Written: " << to_string(*output) << std::endl;
          std::cerr << "Output Written: " << to_string(*output) << " after " << sim_steps << " steps." << std::endl;
        }
        break;
		}

		//if there was no output, do time travel:
		if (tt_dest != -1U) {
        if (!json) {
          std::cout << "Time traveling to ticks[" << tt_dest << "]." << std::endl;
        }
        next = std::move(ticks[tt_dest]);
        writes = std::move(tt_writes);
        ticks.erase(ticks.begin() + tt_dest, ticks.end());
        resolve_writes();
		}

		if (output) {
        if (json) {
          std::cout << ",\n{\"t\":\"output\",\"output\":" << to_string(*output) << ",\"timetravel\":true}";
        }
        else {
          std::cout << "Output Written (during time travel): " << to_string(*output) << std::endl;
          std::cerr << "Output Written (during time travel): " << to_string(*output) << std::endl;
        }
        break;
		}

		ticks.emplace_back(std::move(next));
      if (json) {
        std::cout << "," << std::endl;
      }
      else {
        std::cout << "------ ticks[" << ticks.size() - 1 << "], step " << sim_steps << " ------" << std::endl;
      }
		dump(ticks.back(), json, ticks.size());

		if (!reduced && !json) {
			std::cout << "No operator can reduce." << std::endl;
			break;
		}
      sim_steps++;
      if (limit_steps != -1 && sim_steps >= limit_steps) {
        break;
      }
	}
   }
   catch (const std::runtime_error& e) {
     if (json) {
       std::cout << ",{\"t\": \"error\", \"msg\": \"" << e.what() << "\"}" << std::endl;
     }
     else {
       throw e;
     }
   }

   if (json) {
     std::cout << "]";
   }


	return 0;
}
