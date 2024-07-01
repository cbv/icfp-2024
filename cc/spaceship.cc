
#include "util.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numbers>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ansi.h"
#include "auto-histo.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "bounds.h"
#include "color-util.h"
#include "hashing.h"
#include "image.h"
#include "periodically.h"
#include "timer.h"

static constexpr bool VERBOSE = false;

#define SIMPLE_GREEDY 1

struct Spaceship {
  int x = 0, y = 0;
  int dx = 0, dy = 0;
};

struct Problem {
  static Problem FromFile(const std::string &filename) {
    Problem p;
    for (std::string line : Util::NormalizeLines(
             Util::ReadFileToLines(filename))) {
      int x = atoi(Util::chop(line).c_str());
      int y = atoi(Util::chop(line).c_str());

      CHECK(line.empty()) << line;
      p.stars.emplace_back(x, y);
    }
    return p;
  }

  std::vector<std::pair<int, int>> stars;

  void PrintInfo() {
    std::unordered_set<std::pair<int, int>,
                       Hashing<std::pair<int, int>>>
      unique;
    IntBounds bounds;
    bounds.Bound(0, 0);
    for (const auto &star : stars) {
      unique.insert(star);
      bounds.Bound(star);
    }
    fprintf(stderr,
            AYELLOW("%d") " stars; " ACYAN("%d")
            " distinct. %d x %d\n",
            (int)stars.size(), (int)unique.size(),
            (int)bounds.Width(), (int)bounds.Height());
  }

};

static void Draw(const Problem &p,
                 std::string_view steps,
                 const std::string &filename) {
  Bounds bounds;
  bounds.Bound(0.0, 0.0);
  for (const auto &[x, y] : p.stars) {
    bounds.Bound(x, y);
  }

  bounds.AddMarginFrac(0.1);

  static constexpr int TARGET_SQUARE = 2048;

  // Size of the image square.
  int SQUARE = 128;

  while (SQUARE < TARGET_SQUARE &&
         SQUARE < bounds.Width() * 2 &&
         SQUARE < bounds.Height() * 2) {
    SQUARE <<= 1;
  }

  const int WIDTH = SQUARE;
  const int HEIGHT = SQUARE;

  ImageRGBA img(WIDTH, HEIGHT);
  img.Clear32(0x000000FF);

  Bounds::Scaler scaler = bounds.ScaleToFit(WIDTH, HEIGHT, true);

  // Draw the path first.
  Spaceship ship;
  int prevx = 0, prevy = 0;
  for (uint8_t c : steps) {
    int ax = 0, ay = 0;
    switch (c) {
    case '7': case '8': case '9': ay = +1; break;
    case '4': case '5': case '6': ay = 0; break;
    case '1': case '2': case '3': ay = -1; break;
    default:
      LOG(FATAL) << "bad char " << c;
    }

    switch (c) {
    case '7':
    case '4':
    case '1':
      ax = -1;
      break;

    case '8':
    case '5':
    case '2':
      ax = 0;
      break;

    case '9':
    case '6':
    case '3':
      ax = +1;
      break;

    default:
      LOG(FATAL) << "bad char " << c;
    }

    float angle = (atan2(ay, ax) + std::numbers::pi) / (2.0 * std::numbers::pi);

    uint32_t color = (ax == 0 && ay == 0) ?
      0x888888AA :
      ColorUtil::HSVAToRGBA32(angle, 1.0, 1.0, 0.8);

    // uint8_t r = "\x22\x77\xCC"[ax + 1];
    // uint8_t g = "\x22\x77\xCC"[ay + 1];
    // uint8_t b = 0x55;

    ship.dx += ax; ship.dy += ay;
    ship.x += ship.dx; ship.y += ship.dy;

    {
      const auto &[sprevx, sprevy] = scaler.Scale(prevx, prevy);
      const auto &[screenx, screeny] = scaler.Scale(ship.x, ship.y);
      img.BlendLine32(sprevx, sprevy, screenx, screeny, color);
      img.BlendPixel32(screenx, screeny, color | 0xFF);
    }

    prevx = ship.x;
    prevy = ship.y;
  }

  // Draw stars on top of the path, so you can see 'em.
  for (const auto &[x, y] : p.stars) {
    const auto &[screenx, screeny] = scaler.Scale(x, y);
    img.BlendFilledCircle32(screenx, screeny, 2, 0xAAAAFFCC);
  }

  // Scale if small
  int scaleup = TARGET_SQUARE / SQUARE;
  if (scaleup > 1) {
    img = img.ScaleBy(scaleup);
  }

  img.Save(filename);
  fprintf(stderr, "Wrote %s\n", filename.c_str());
};

struct Solver {
  std::string solution;

  std::unordered_set<std::pair<int, int>,
                     Hashing<std::pair<int, int>>> unique;

  Spaceship ship;

  AutoHisto histo;

  int maxdx = 0, maxdy = 0;

  using TableValue = uint32_t;
  static constexpr TableValue POSSIBLE   = uint32_t{0b11000000} << 24;
  static constexpr TableValue IMPOSSIBLE = uint32_t{0b10000000} << 24;

  // ax, ay, time_steps
  static std::optional<std::tuple<int, int, int>> Decode(uint32_t v) {
    if (v & POSSIBLE) {
      uint8_t b = v >> 24;
      return {std::make_tuple(((b >> 2) & 3) - 1,
                              (b & 3) - 1,
                              v & 0x00FFFFFF)};
    } else {
      return std::nullopt;
    }
  }

  static constexpr TableValue Encode(int ax, int ay, int steps) {
    // CHECK(ax >= -1 && ax <= 1);
    // CHECK(ay >= -1 && ay <= 1);
    CHECK(steps >= 0 && steps < 0x00FFFFFF) << steps;
    // Encoded byte always has high bit set so that we can tell
    // when we got the default 0.
    uint8_t d = ((ax + 1) << 2) | (ay + 1);
    uint32_t r = POSSIBLE | (d << 24) | steps;

    if (false) {
      auto ao = Decode(r);
      CHECK(ao.has_value());
      const auto [aax, aay, ss] = ao.value();
      CHECK(aax == ax && aay == ay && ss == steps);
    }
    return r;
  }

  // Same data, but dense.
  // 64^4 is 16 MB.
  // We could save half this with encoding tricks since
  // we know dx < dy.
  static constexpr int DENSE_WIDTH = 62;
  std::vector<uint32_t> dense_table;
  uint32_t &DenseTableAt(int vx, int vy, int dx, int dy) {
    int idx = 0;
    idx *= DENSE_WIDTH; idx += vx;
    idx *= DENSE_WIDTH; idx += vy;
    idx *= DENSE_WIDTH; idx += dx;
    idx *= DENSE_WIDTH; idx += dy;
    return dense_table[idx];
  }


  // Sparse table, when any arg is >= DENSE_WIDTH.
  std::unordered_map<std::tuple<int, int, int, int>,
                     uint32_t,
                     Hashing<std::tuple<int, int, int, int>>> table;

  // For nonnegative distances (dx, dy). Gives the next acceleration
  // step to reach the distance at the same time, trying to maximize
  // velocity. Returns IMPOSSIBLE if we will definitely overshoot.
  TableValue Tabled(int vx, int vy, int dx, int dy) {
    table_calls++;
    if (!(table_calls & 0xFFFFFF)) {
      fprintf(stderr, "C %lld, d %lld, s %lld, f %lld, h %lld [%d %d %d %d]\n",
              table_calls, dense_calls, fast_calls,
              (int64_t)table.size(),
              table_hits,
              vx, vy, dx, dy);
    }

    // Since the table is symmetric, we only compute it for dx<=dy.
    if (dx <= dy) {
      return NormalTabled(vx, vy, dx, dy);
    } else {
      if (VERBOSE) fprintf(stderr, "Swap\n");
      TableValue swap_val = NormalTabled(vy, vx, dy, dx);
      auto ao = Decode(swap_val);
      if (ao.has_value()) {
        const auto &[ax, ay, t] = ao.value();
        return Encode(ay, ax, t);
      } else {
        return IMPOSSIBLE;
      }
    }
  }

  int64_t table_calls = 0, table_hits = 0, dense_calls = 0,
    fast_calls = 0;
  TableValue NormalTabled(int vx, int vy, int dx, int dy) {
    CHECK(dx <= dy) << "precondition";

    if (dx < 0 || dy < 0) return IMPOSSIBLE;

    // Avoid doing any lookups for simple edge cases.

    if (dx == 0 && dy == 0) {
      // Done!
      fast_calls++;
      return Encode(0, 0, 0);
    }

    {
      int ax = dx - vx;
      int ay = dy - vy;

      if (VERBOSE && !(table_calls & 0xFFFFFF)) {
        fprintf(stderr, ".. %d %d\n", ax, ay);
      }
      if (abs(ax) <= 1 && abs(ay) <= 1) {
        if (VERBOSE && !(table_calls & 0xFFFFFF)) {
          fprintf(stderr, "  FAST\n");
        }

        // We can get the exact velocity on the current time step, so this
        // is clearly optimal.
        fast_calls++;
        return Encode(ax, ay, 1);
      }
    }

    uint32_t *val = nullptr;

    if (vx >= 0 && vx < DENSE_WIDTH &&
        vy >= 0 && vy < DENSE_WIDTH &&
        dx >= 0 && dx < DENSE_WIDTH &&
        dy >= 0 && dy < DENSE_WIDTH) {
      val = &DenseTableAt(vx, vy, dx, dy);
      dense_calls++;
    } else{
      val = &table[std::make_tuple(vx, vy, dx, dy)];
    }

    if (*val != 0) {
      table_hits++;
      return *val;
    }

    // Try a single acceleration value. In the general case we try
    // them all, but below we cut off the search in cases where we
    // obviously have to turn around, for example.
    std::optional<int> best_t;
    uint32_t best_val = IMPOSSIBLE;
    auto Try = [this, &best_t, &best_val, vx, vy, dx, dy](int ax, int ay) {
        // accelerate before moving
        int nvx = vx + ax;
        int nvy = vy + ay;

        int ndx = dx - vx;
        int ndy = dy - vy;

        if (VERBOSE)
        fprintf(stderr, "try(%d,%d) giving %d %d %d %d\n", ax, ay,
                nvx, nvy, ndx, ndy);

        CHECK(nvx != vx || nvy != vy || ndx != dx || ndy != dy);
        TableValue nval = Tabled(nvx, nvy, ndx, ndy);
        auto ao = Decode(nval);

        if (ao.has_value()) {
          const auto &[ax_next, ay_next, t] = ao.value();
          // Smaller time is better.
          if (!best_t.has_value() || t < best_t.value()) {
            best_t = {t};
            // We don't actually care what the value is (it tells us
            // what to do in *that* state). Our best is the encoding
            // of the acceleration on this step. It takes one more
            // time step.
            best_val = Encode(ax, ay, t + 1);
          }
        }
      };


    if (vx < 0 && vy < 0) {
      // Both velocities have to pass through zero.
      // So we should always decelerate both.
      if (VERBOSE) fprintf(stderr, "decel both\n");
      Try(+1, +1);

    } else if (vy < 0) {
      if (VERBOSE) fprintf(stderr, "vy < 0\n");
      // ay will definitely be 1. Search for ax.
      for (int ax : {0, 1}) {
        Try(ax, +1);
      }

    } else if (vx < 0) {
      if (VERBOSE) fprintf(stderr, "vx < 0\n");
      // Symmetric...
      for (int ay : {0, 1}) {
        Try(+1, ay);
      }

    } else if (vx == 0 && vy == 0) {

      // Make sure we don't loop forever when we don't accelerate and
      // aren't moving. Also, don't go backwards when the target is
      // in front of us.
      if (dx > 0 && dy > 0) {
        Try(1, 1);
      } else if (dx == 0) {
        CHECK(dy != 0);
        Try(0, 1);
      } else if (dy == 0) {
        CHECK(dx != 0);
        Try(1, 0);
      }

    } else if (vx > dx && vy > dy) {
      // If we'll certainly overshoot, only turn around until we reach a zero.
      Try(-1, -1);

    } else if (vx > dx) {
      // Overshoot on x. Only turn around on x.
      for (int ax : {-1, 0}) {
        Try(ax, -1);
      }

    } else if (vy > dy) {
      // Need to slow down on y axis.
      for (int ax : {-1, 0}) {
        Try(ax, -1);
      }

    } else if (vx == 0 && dx == 0) {
      for (int ay : {-1, 0, 1}) {
        Try(0, ay);
      }

    } else if (vy == 0 && dy == 0) {
      for (int ax : {-1, 0, 1}) {
        Try(ax, 0);
      }

    } else {
      // Otherwise, try all of the possibilities.
      if (VERBOSE) fprintf(stderr, "general case\n");
      for (int ay : {-1, 0, 1}) {
        for (int ax : {-1, 0, 1}) {
          Try(ax, ay);
        }
      }

    }

    // Memoize it and return. It might be IMPOSSIBLE; otherwise it
    // is the step that gives us the highest velocity and still reaches
    // the position exactly.
    *val = best_val;
    return best_val;
  }

  // For nonnegative v, d.
  static double TimeToDistNonNeg(double v, double d) {
    CHECK(v >= 0.0);
    CHECK(d >= 0.0);
    // t = -v +/- sqrt(v^2 + 2 * a * d)/a
    // and a=1
    double s = sqrt(v * v + 2 * d);
    double t0 = -v + s;
    double t1 = -v - s;

    CHECK(t0 >= 0.0 || t1 >= 0.0) <<
      "Maybe d or v is negative? v: " << v << " d: " << d;
    if (t0 < 0.0) return t1;
    if (t1 < 0.0) return t0;
    // Probably impossible?
    return std::min(t0, t1);
  }

  static double TimeToDist1D(double v, double d) {
    // make d nonnegative, by reflecting
    if (d < 0.0) {
      d = -d;
      v = -v;
    }

    CHECK(d >= 0.0);

    // If we're headed the wrong way, first we need to stop
    if (v < 0.0) {
      // How far away (p) do we get when the final velocity (vf) = 0?
      // p = (vf^2 - v^2) / 2a
      // p = -v^2 / 2
      double p = v * v / -2;
      // p is negative, but we are increasing the dist.
      double effective_dist = d - p;
      // PERF: Could specialize this since we know v is 0.
      return TimeToDistNonNeg(0.0, effective_dist);
    } else {
      return TimeToDistNonNeg(v, d);
    }
  }

  // Various distances, just used for picking the "closest".
  static int64_t DistSqEuclidean(const Spaceship &ship, int x1, int y1) {
    int64_t dxx = x1 - ship.x;
    int64_t dyy = y1 - ship.y;
    return dxx * dxx + dyy * dyy;
  }

  // Treating each axis separately, find how long it would take us
  // (milliticks) to reach the point by constantly accelerating
  // towards it. This is optimistic because we aren't moving
  // continuously.
  static int64_t DistConstantAccelSeparate(const Spaceship &ship, int x1, int y1) {
    return std::max(TimeToDist1D(ship.dx, x1 - ship.x),
                    TimeToDist1D(ship.dy, y1 - ship.y)) * 1000.0;
  }

  // Pick a target node (removing it from the tree).
  std::pair<int, int> GetTarget() {
    // Just using the closest for now. But really
    // we should be taking the current velocity into account!
    // const auto &[star_pos, val_, dist_] = tree.Closest({sx, sy});
    // CHECK(tree.Remove(star_pos));

    CHECK(!unique.empty());

    #if SIMPLE_GREEDY

    std::pair<int, int> star_pos = {0, 0};
    std::optional<int64_t> best_dist;
    for (const auto &star : unique) {
      int64_t dist = DistSqEuclidean(ship, star.first, star.second);
      if (!best_dist.has_value() || dist < best_dist.value()) {
        best_dist = dist;
        star_pos = {star};
      }
    }

    #else
    // Sort everything by the quick metric.
    std::vector<std::pair<std::pair<int, int>, int64_t>> scored;
    scored.reserve(unique.size());

    for (const auto &star : unique) {
      int64_t dist =
        // DistSqEuclidean
        DistConstantAccelSeparate
        (ship, star.first, star.second);

      scored.emplace_back(star, dist);
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto &a, const auto &b) {
                return a.second < b.second;
              });

    // Now compute the actual path (for the best scoring ones) using
    // the online planner.
    static constexpr int MAX_TO_SCORE = 16;
    if ((int)scored.size() > MAX_TO_SCORE) scored.resize(MAX_TO_SCORE);

    std::pair<int, int> star_pos = {0, 0};
    std::optional<int64_t> best_dist;
    for (const auto &[star, quick_dist] : scored) {
      int64_t dist = DistTo(ship, star);
      if (!best_dist.has_value() || dist < best_dist.value()) {
        best_dist = dist;
        star_pos = {star};
      }
    }
    #endif

    CHECK(best_dist.has_value());
    histo.Observe(best_dist.value());

    unique.erase(star_pos);

    return star_pos;
  }

  explicit Solver(Problem p) {
    for (const auto &pt : p.stars) {
      unique.insert(pt);
    }

    dense_table.resize(
        DENSE_WIDTH * DENSE_WIDTH * DENSE_WIDTH * DENSE_WIDTH,
        0);
  }

  // Move to every spot, appending to the solution.
  void Solve() {
    Periodically status_per(1.0);
    Timer timer;

    const int total = (int)unique.size();
    int done = 0;
    while (!unique.empty()) {
      auto star_pos = GetTarget();

      // Go to the point.
      GoTo(star_pos);
      done++;

      if (status_per.ShouldRun()) {
        fprintf(stderr, ANSI_UP "%s\n",
                ANSI::ProgressBar(
                    done, total,
                    StringPrintf("@%d,%d ^[%d,%d] sol %d",
                                 ship.x, ship.y, ship.dx, ship.dy,
                                 (int)solution.size()),
                    timer.Seconds()).c_str());
      }
    }
  }

  // On one axis, do we want to accelerate, coast, or decelerate?
  // Assumes dist is nonnegative. Returns only 1, 0, or -1.
  std::pair<int, int> PedalPos2D(int vx, int vy, int x, int y) {
    // Don't mess with it if it's perfect!
    if (vx == x && vy == y)
      return {0, 0};

    // Definitely turn around if we're moving in completely the wrong
    // direction.
    if (vx < 0 && vy < 0) {
      return {+1, +1};
    }

    // We will overshoot, so we should get a head start on
    // deceleration.
    if (vx > x && vy > y) {
      return {-1, -1};
    }

    // Now we have a nontrivial problem with nonnegative distance. Use
    // the tabled solution for this:
    TableValue val = Tabled(vx, vy, x, y);
    const auto ao = Decode(val);
    if (ao.has_value()) {
      // Can we use t for something?
      const auto &[ax, ay, t_] = ao.value();
      return {ax, ay};
    } else {
      // Slow down. We know there is always a solution by moving (1,0) and (0,1).
      CHECK(x >= 0 && y >= 0 &&
            vx >= 0 && vy >= 0) << "Established above";
      int ax = 0;
      int ay = 0;
      if (vx > 1) ax = -1;
      if (vy > 1) ay = -1;
      return {ax, ay};
    }
  }

  // As above, but supports any sign of distance.
  std::pair<int, int> Pedal2D(const Spaceship &ship, const std::pair<int, int> &star) {
    const auto &[star_x, star_y] = star;

    const int distx = star_x - ship.x;
    const int disty = star_y - ship.y;

    const int signx = distx < 0 ? -1 : 1;
    const int signy = disty < 0 ? -1 : 1;

    const int dx = ship.dx * signx;
    const int dy = ship.dy * signy;

    const auto &[ax, ay] = PedalPos2D(dx, dy, signx * distx, signy * disty);

    return std::make_pair(signx * ax, signy * ay);
  }

  // Like the above, but solve both dimensions at once.
  template<class F>
  Spaceship PathTo2D(
      Spaceship ship,
      std::pair<int, int> star_pos,
      const F &emit) {

    for (;;) {
      if (ship.x == star_pos.first &&
          ship.y == star_pos.second) {
        // We hit the star, so we're done.
        return ship;
      }

      const auto &[ax, ay] = Pedal2D(ship, star_pos);

      ship.dx += ax;
      ship.dy += ay;

      ship.x += ship.dx;
      ship.y += ship.dy;

      emit(ship, ax, ay);
    }
  }


  // Generate the path to the point, just for the sake of measuring its length.
  int DistTo(Spaceship ship, std::pair<int, int> star_pos) {
    int count = 0;
    (void)PathTo2D(ship, star_pos, [&count](auto, auto, auto) { count++; });
    return count;
  }

  // Generate the path to the point, and update the state/solution with it.
  void GoTo(std::pair<int, int> star_pos) {
    ship =
      PathTo2D(ship, star_pos, [this](const Spaceship &ship, int ax, int ay) {
          if (abs(ship.dx) > maxdx) maxdx = abs(ship.dx);
          if (abs(ship.dy) > maxdy) maxdy = abs(ship.dy);

          solution.push_back(Key(ax, ay));
        });
  }

  static char Key(int ax, int ay) {
    CHECK(ax >= -1 && ax <= 1);
    CHECK(ay >= -1 && ay <= 1);
    switch (ay) {
    case +1: return "789"[ax + 1];
    case  0: return "456"[ax + 1];
    case -1: return "123"[ax + 1];
    default: return 'X';
    }
  }

};

int main(int argc, char **argv) {

  int n = argc == 2 ? atoi(argv[1]) : 0;
  CHECK(n > 0 && n < 100) <<
    "spaceship.exe n\n"
    "... where n is the problem number.\n"
    "(Run from the cc dir)\n";

  std::string file =
    StringPrintf("../puzzles/spaceship/spaceship%d.txt", n);

  Problem p = Problem::FromFile(file);
  CHECK(!p.stars.empty()) << file;
  p.PrintInfo();

  Solver solver(p);
  solver.Solve();
  fprintf(stderr,
          "\nSolved " AYELLOW("%d") " in " AGREEN("%d") " moves. Max velocity: ["
          ACYAN("%d") "," ACYAN("%d") "]\n\n\n",
          n, (int)solver.solution.size(),
          solver.maxdx, solver.maxdy);

  Draw(p, solver.solution,
       StringPrintf("spaceship%d.png", n));

  std::string h = solver.histo.SimpleANSI(32);
  fprintf(stderr, "Steps between stars:\n%s\n\n\n", h.c_str());

  printf("solve spaceship%d %s\n",
         n,
         solver.solution.c_str());

  return 0;
}
