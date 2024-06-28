#include <stdio.h>

#include <math.h>

#include "base/logging.h"
#include "geom/bezier.h"

using namespace std;

#define CHECK_FEQ(a, b) CHECK(fabs((a) - (b)) < 0.00001)

static void DistanceToQuad() {
  const auto [x, y, d] =
    DistanceFromPointToQuadBezier(
        // The point to test
        1.0f, 2.0f,
        // Bezier start point
        2.0f, -1.0f,
        // Bezier ontrol point
        -1.0f, 4.0f,
        // Bezier end point
        4.0f, 5.0f);

  CHECK_FEQ(0.88100123f, x);
  CHECK_FEQ(1.99277639f, y);
  CHECK_FEQ(0.01421289f, d);

  // TODO: Test endpoints, etc.
}

int main(int argc, char **argv) {
  DistanceToQuad();

  printf("OK\n");
  return 0;
}

