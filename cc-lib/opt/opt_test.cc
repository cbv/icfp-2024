#include <stdio.h>

#include <math.h>

#include "opt/opt.h"
#include "base/logging.h"

// sin(x^3 - 3x + 3) + x^2
// The parabola takes over quickly, with a single global
// minimum around -0.467979 (Wolfram Alpha), but lots of
// local minima.
static double Test1(const std::array<double, 1> &args) {
  double x = args[0];
  return sin(x * x * x - 3.0 * x + 3) + x * x;
}

static double Test1v(const std::vector<double> &args) {
  CHECK(args.size() == 1);
  double x = args[0];
  return sin(x * x * x - 3.0 * x + 3) + x * x;
}

static double Test1ez(double x) {
  return sin(x * x * x - 3.0 * x + 3) + x * x;
}

static double Test2(double x, double y) {
 return x * x + 17.0 * sin(x + 27.5) +
   5.0 * cos((y - 13.0) / 5.0) +
   sqrt((1.7 * y) * (1.7 * y));
}

// zero at (1, -1, 4)
static double Test3(double x, double y, double z) {
  double xx = (x - 1.0);
  double yy = (y + 1.0);
  double zz = (z / 8.0 - 0.5);

  return xx * xx + yy * yy + zz * zz * zz * zz;
}

static double Bounds(double x, double y, double z) {
  CHECK(x >= 0.0 && x <= 3.0);
  CHECK(y >= 0.1 && y <= 10.0);
  CHECK(z >= 0.0 && z <= 6.0);

  return x + y + z;
}

int main(int argc, char **argv) {

  {
    const auto [args, v] =
      Opt::Minimize<1>(Test1, {-1000.0}, {1000.0}, 1000);

    printf("Found minimum at f(%.5f) = %.5f\n"
           "(Expected f(%.5f) = %.5f)\n",
           args[0],
           v,
           -0.467979,
           Test1({-0.467979}));
  }

  {
    const auto [vargs, vv] =
      Opt::Minimize(1, Test1v, {-1000.0}, {1000.0}, 1000);

    CHECK(vargs.size() == 1);
    printf("Vector version: f(%.5f) = %.5f\n",
           vargs[0], vv);
  }

  {
    const auto [arg, vv] =
      Opt::Minimize1D(Test1ez, -1000.0, 1000.0, 1000);

    printf("EZ overload: f(%.5f) = %.5f\n",
           arg, vv);
  }

  {
    auto [aarg, v] = Opt::Minimize2D(
        Test2,
        {-1000.0, -1000.0},
        {+1000.0, +1000.0},
        1000,
        1, 10);
    auto [x, y] = aarg;
    printf("EZ overload 2D: f(%.5f, %.5f) = %.5f\n",
           x, y, v);
  }

  {
    auto [aarg, v] = Opt::Minimize3D(
        Test3,
        {-50.0, -50.0, -50.0},
        {+50.0, +50.0, +50.0},
        1000,
        1, 10);
    auto [x, y, z] = aarg;
    printf("3D. Minimum at (1, -1, 4); got: (%.5f, %.5f, %.5f)\n",
           x, y, z);
  }

  {
    auto [aarg, v] = Opt::Minimize3D(
        Bounds,
        {0.0, 0.1, 0.0},
        {3.0, 10.0, 6.0},
        1000,
        1, 100);
    auto [x, y, z] = aarg;
    printf("3D. Minimum at (0, 0.1, 0.0); got: (%.5f, %.5f, %.5f)\n",
           x, y, z);
  }


  #if 0
  {
    // 2D, bare function interface
    // auto [args, v] =

    std::function<double(double, double)> ff = Test2;

    (void)
      Opt::MinimizeF(
          ff,
          {-1000.0, -1000.0},
          {+1000.0, +1000.0},
          1000,
          1, 10);
    /*

    auto [x, y] = args;
    printf("Bare function version 2D: f(%.5f, %.5f) = %.5f\n",
           x, y, v);
    */
  }
  #endif

  return 0;
}

