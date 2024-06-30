
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>
#include <cstdint>
#include <string>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "util.h"
#include "ansi.h"
#include "image.h"
#include "color-util.h"

static constexpr ColorUtil::Gradient RAINBOW{
  GradRGB(0.0f, 0x440000),
  GradRGB(0.2f, 0x7700BB),
  GradRGB(0.3f, 0xFF0000),
  GradRGB(0.4f, 0xFFFF00),
  GradRGB(0.5f, 0xFFFFFF),
  GradRGB(0.7f, 0x00FF33),
  GradRGB(1.0f, 0x0000FF),
};


struct Board {
  int width = 0;
  int height = 0;
  int dots = 0;
  // '.' = pac-dot
  // ' ' = empty
  // '#' = wall, never touched
  // '@' = wall that you collided with
  std::vector<uint8_t> cells;
  uint8_t &At(int x, int y) {
    CHECK(x >= 0 && x < width &&
          y >= 0 && y < height);
    return cells[y * width + x];
  }

  uint8_t At(int x, int y) const {
    CHECK(x >= 0 && x < width &&
          y >= 0 && y < height);
    return cells[y * width + x];
  }

  int lx = 0, ly = 0;

  // Save image of the board state after the solution, including the path
  // drawn by the solution.
  void SaveImage(const std::string &filename, int scale = 7,
                 const std::string &sol = "") {
    ImageRGBA img(width * scale, height * scale);
    img.Clear32(0x111122FF);

    // Play the solution, which also gives me the (simple) path.
    const int startx = lx, starty = ly;
    std::string simple_sol = Play(sol);

    // Draw board first.

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        uint8_t val = At(x, y);
        switch (val) {
        case ' ':
          break;
        case '#':
        case '@': {
          uint32_t color = val == '#' ? 0xAAAAAAFF : 0xDDDDDDFF;
          img.BlendBox32(x * scale + 1, y * scale + 1, scale - 2, scale - 2,
                         color, {color & 0xFFFFFF99});
          if (scale - 4 > 0) {
            img.BlendRect32(x * scale + 2, y * scale + 2, scale - 4, scale - 4,
                            color);
          }
          break;
        }
        case '.':
          img.BlendFilledCircleAA32((x + 0.5f) * scale,
                                    (y + 0.5f) * scale,
                                    scale * 0.3f, 0xAAAA22FF);
          break;
        default:
          img.BlendBox32(x * scale, y * scale, scale, scale, 0xFF0000FF,
                         0xFF0000FF);
          break;
        }
      }
    }

    img.BlendBox32(lx * scale + 1, ly * scale + 1, scale - 2, scale - 2,
                   0xFF00FFFF, {0xFF00FFAA});
    if (scale - 4 > 0) {
      img.BlendRect32(lx * scale + 2, ly * scale + 2, scale - 4, scale - 4,
                      0xFF00FFFF);
    }

    // Now draw path.
    double denom = simple_sol.size();
    int cx = startx, cy = starty;
    int ox = cx, oy = cy;
    for (int i = 0; i < (int)simple_sol.size(); i++) {
      const uint8_t c = simple_sol[i];
      int dx = 0, dy = 0;
      switch (c) {
      case 'U': dy = -1; break;
      case 'L': dx = -1; break;
      case 'D': dy = +1; break;
      case 'R': dx = +1; break;
      default:
        LOG(FATAL) << "Impossible";
      }

      // current color in gradient
      uint32_t color = ColorUtil::LinearGradient32(RAINBOW, i / denom);

      // draw a line
      // TODO: jitter?
      cx += dx;
      cy += dy;
      img.BlendLine32(ox * scale + (scale / 2),
                      oy * scale + (scale / 2),
                      cx * scale + (scale / 2),
                      cy * scale + (scale / 2),
                      color & 0xFFFFFF99);
      ox = cx;
      oy = cy;
    }

    img.Save(filename);
  }

  std::string Play(std::string_view s) {
    std::string out;
    for (char c : s) {
      int dx = 0, dy = 0;

      switch (c) {
      case 'U': dx = 0;  dy = -1; break;
      case 'D': dx = 0;  dy = +1; break;
      case 'L': dx = -1; dy = 0; break;
      case 'R': dx = +1; dy = 0; break;
      default: LOG(FATAL) << "Bad solution char " << c;
      }

      uint8_t val = At(lx + dx, ly + dy);
      if (val == '#' || val == '@') {
        At(lx + dx, ly + dy) = '@';
        // Lambda man stays still. (And don't copy it to output!)
      } else {
        out.push_back(c);
        lx += dx;
        ly += dy;
        if (At(lx, ly) == '.') {
          dots--;
          At(lx, ly) = ' ';
        }
      }
    }
    return out;
  }
};

static Board FromFile(const std::string &filename) {
  std::vector<std::string> lines =
    Util::NormalizeLines(Util::ReadFileToLines(filename));


  Board board;
  board.height = 2 + (int)lines.size();
  const int line_width = (int)lines[0].size();
  for (const std::string &line : lines) {
    CHECK((int)line.size() == line_width) << "Want lines that "
      "are all the same length";
  }
  board.width = 2 + (int)lines[0].size();
  board.cells.resize(board.width * board.height, '#');

  // We place the board at (1,1) so that it can be surrounded by
  // walls.
  for (int y = 0; y < (int)lines.size(); y++) {
    for (int x = 0; x < line_width; x++) {
      uint8_t c = lines[y][x];
      switch (c) {
      case '#':
        board.At(x + 1, y + 1) = '#';
        break;
      case '.':
        board.At(x + 1, y + 1) = '.';
        board.dots++;
        break;
      case 'L':
        board.At(x + 1, y + 1) = ' ';
        board.lx = x + 1;
        board.ly = y + 1;
        break;
      default:
        LOG(FATAL) << "Unknown character in input: "
                   << StringPrintf("'%c' 0x%02x", c, c);
        break;
      }
    }
  }

  fprintf(stderr, "Lambda man at %d,%d. %d dots.\n",
          board.lx, board.ly, board.dots);

  return board;
}

[[maybe_unused]]
static void Solve21() {
  Board board = FromFile("../puzzles/lambdaman/lambdaman21.txt");

  std::string soln;

  auto U = [&](int num) {
      std::string s = std::string(num, 'U');
      soln += board.Play(s);
    };
  auto D = [&](int num) {
      std::string s = std::string(num, 'D');
      soln += board.Play(s);
    };
  auto L = [&](int num) {
      std::string s = std::string(num, 'L');
      soln += board.Play(s);
    };
  auto R = [&](int num) {
      std::string s = std::string(num, 'R');
      soln += board.Play(s);
    };

  // Move the maximum amount, but it will get short
  // by some wall.
  [[maybe_unused]] auto UU = [&]() { U(200); };
  [[maybe_unused]] auto DD = [&]() { D(200); };
  [[maybe_unused]] auto LL = [&]() { L(200); };
  [[maybe_unused]] auto RR = [&]() { R(200); };

  // Go to 1,1
  U(72); L(77);

  // Strafe-fill top.
  for (int i = 0; i < 23; i++) {
    R(200); D(1); L(200); D(1);
  }

  U(1); L(19); D(1); LL();

  // Test solution.
  {
    Board board = FromFile("../puzzles/lambdaman/lambdaman21.txt");
    board.Play(soln);
    board.SaveImage("l21.png");
  }
}



int main(int argc, char **argv) {
  ANSI::Init();
  CHECK(argc == 5) << "./lambdaman.exe scale puzzle.txt solution.txt out.png\n"
    "Scale is the number of pixels per cell; I recommend at least 5.\n"
    "These use plain text and do no evaluation.";

  int scale = atoi(argv[1]);
  CHECK(scale > 0) << "Scale must be positive!";
  Board board = FromFile(argv[2]);
  std::string soln = Util::ReadFile(argv[3]);
  CHECK(soln.find("solve lambdaman") == 0) << "I want it to have the solve marker "
    "so that I can remove it.";
  (void)Util::chop(soln);
  (void)Util::chop(soln);
  soln = Util::NormalizeWhitespace(soln);

  std::string out = argv[4];

  board.SaveImage(out, scale, soln);

  // Solve21();

  return 0;
}
