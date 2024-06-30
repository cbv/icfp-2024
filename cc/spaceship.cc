
#include "util.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <cstdlib>
#include <utility>
#include <vector>

#include "ansi.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "bounds.h"
#include "geom/tree-2d.h"
#include "hashing.h"
#include "periodically.h"
#include "timer.h"
#include "image.h"
#include "color-util.h"
#include "auto-histo.h"

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

    uint8_t r = "\x22\x77\xCC"[ax + 1];
    uint8_t g = "\x22\x77\xCC"[ay + 1];
    uint8_t b = 0x22;

    ship.dx += ax; ship.dy += ay;
    ship.x += ship.dx; ship.y += ship.dy;

    {
      const auto &[sprevx, sprevy] = scaler.Scale(prevx, prevy);
      const auto &[screenx, screeny] = scaler.Scale(ship.x, ship.y);
      img.BlendLine(sprevx, sprevy, screenx, screeny, r, g, b, 0xAA);
      img.BlendPixel(screenx, screeny, r, g, b, 0xFF);
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

struct Unit { };
std::ostream& operator<<(std::ostream& os, const Unit& unit) {
  return os;
}

struct Solver {
  std::string solution;

  using Tree = Tree2D<int, Unit>;
  Tree tree;

  std::unordered_set<std::pair<int, int>,
                     Hashing<std::pair<int, int>>> unique;

  Spaceship ship;

  AutoHisto histo;

  int maxdx = 0, maxdy = 0;

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

    CHECK(best_dist.has_value());
    histo.Observe(best_dist.value());

    unique.erase(star_pos);

    return star_pos;
  }

  explicit Solver(Problem p) {
    for (const auto &pt : p.stars) {
      unique.insert(pt);
    }

    /*
    for (const auto &pt : unique) {
      tree.Insert(pt, Unit{});
    }

    tree.DebugPrint();
    */
  }

  // Move to every spot, appending to the solution.
  void Solve() {
    Periodically status_per(1.0);
    Timer timer;

    const int total = (int)unique.size();
    int done = 0;
    while (!unique.empty()) {
      auto star_pos = GetTarget();
      /*
      fprintf(stderr, "Next " AYELLOW("*") "[%d,%d] (%d steps)\n",
              star_pos.first, star_pos.second,
              (int)solution.size());
      */

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
  static int PedalPos(int speed, int dist) {
    // Perfect :)
    if (speed == dist)
      return 0;

    // If we're headed in the wrong direction, turn around.
    if (speed < 0)
      return +1;

    // We will overshoot, so we should get a head start on
    // deceleration.
    if (speed > dist)
      return -1;

    // If we're stopped, since we know we aren't already there,
    // it must be positive and so we should start moving.
    if (speed == 0) {
      CHECK(dist > 0);
      return +1;
    }

    // Now figure out how many timesteps it will take us to
    // go the distance at the current speed.

    int steps = dist / speed;
    int err = steps * speed - dist;

    const bool can_reach = [speed, dist]() {
        int s = speed;
        int d = dist;
        // Assume we keep accelerating. Do we reach another
        // divisor before overshooting?
        for (;;) {
          if (d % s == 0) return true;
          if (d <= 0) return false;
          d -= s;
          s++;
        }
      }();

    // If err is 0, then we'll at least hit the target without
    // correction, so we might stay at the current speed.
    if (err == 0) {
      // But if we have enough time to get to the next divisor,
      // we should do that (e.g. 1 divides everything!)

      if (can_reach)
        return +1;

      return 0;
    }

    // Otherwise, we can either speed up or slow down to get
    // in sync. Prefer speeding up if possible, since then
    // we do things faster.
    if (can_reach)
      return +1;

    // We will overshoot at the current speed and can't reach
    // the next divisor, so slow down. This doesn't guarantee
    // we slow down enough, but it makes progress on turning
    // around if we do not!
    return -1;
  }

  // As above, but supports any sign of distance.
  static int Pedal(int speed, int dist) {
    if (dist < 0) {
      return -PedalPos(-speed, -dist);
    } else {
      return PedalPos(speed, dist);
    }
  }

  template<class F>
  static Spaceship PathTo(
      Spaceship ship,
      std::pair<int, int> star_pos,
      const F &emit) {

    for (;;) {
      if (ship.x == star_pos.first &&
          ship.y == star_pos.second) {
        // We hit the star, so we're done.
        return ship;
      }

      int ax = Pedal(ship.dx, star_pos.first - ship.x);
      int ay = Pedal(ship.dy, star_pos.second - ship.y);

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
    (void)PathTo(ship, star_pos, [&count](auto, auto, auto) { count++; });
    return count;
  }

  // Generate the path to the point, and update the state/solution with it.
  void GoTo(std::pair<int, int> star_pos) {
    ship =
      PathTo(ship, star_pos, [this](const Spaceship &ship, int ax, int ay) {
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
          ACYAN("%d") "," ACYAN("%d") "]\n",
          n, (int)solver.solution.size(),
          solver.maxdx, solver.maxdy);

  Draw(p, solver.solution,
       StringPrintf("spaceship%d.png", n));

  std::string h = solver.histo.SimpleANSI(32);
  fprintf(stderr, "Steps between stars:\n%s\n", h.c_str());

  printf("solve spaceship%d %s\n",
         n,
         solver.solution.c_str());

  return 0;
}
