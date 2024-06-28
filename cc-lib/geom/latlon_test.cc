#include <stdio.h>

#include <math.h>

#include "base/logging.h"
#include "geom/latlon.h"

using namespace std;

#define CHECK_FEQ(a, b) CHECK(fabs((a) - (b)) < 0.00001)

static void TestPlusMinusMod() {

  {
    const auto [lat, lon] = LatLon::FromDegs(0.0, 0.0).ToDegs();
    CHECK_FEQ(lat, 0.0);
    CHECK_FEQ(lon, 0.0);
  }

  {
    const auto [lat, lon] = LatLon::FromDegs(91.0, 181.0).ToDegs();
    CHECK_FEQ(lat, -89.0);
    CHECK_FEQ(lon, -179.0);
  }

  {
    const auto [lat, lon] = LatLon::FromDegs(-91.0, -181.0).ToDegs();
    CHECK_FEQ(lat, 89.0) << lat;
    CHECK_FEQ(lon, 179.0) << lon;
  }


}

static void TestParse() {
  auto llo = LatLon::FromString("13.3,-27.777");
  CHECK(llo.has_value());
  auto [lat, lon] = llo.value().ToDegs();
  CHECK_FEQ(lat, 13.3) << lat;
  CHECK_FEQ(lon, -27.777) << lon;

  CHECK(!LatLon::FromString("").has_value());
  CHECK(!LatLon::FromString("a,b").has_value());
  CHECK(!LatLon::FromString("1,2,3").has_value());
  CHECK(!LatLon::FromString(",1.00000").has_value());
  CHECK(!LatLon::FromString(",1.00000").has_value());

}

// Test a short distance and a long one.
static void TestDistance() {
  // Two corners of a tennis court on CMU's campus. Nominally
  // 78 feet apart. I got these coordinates from Google Earth,
  // but note:
  //   - The max precision reported is getting close to the
  //     scale being tested (inches)
  //   - There appears to be a minor bug where when Earth does
  //     not have focus, it interprets the mouse as being in
  //     a slightly different position than when it is in focus.
  //     Copy down the lat/lon values with Earth in focus.
  LatLon tennis1 = LatLon::FromDegs(40.442661, -79.941922);
  LatLon tennis2 = LatLon::FromDegs(40.442455, -79.942001);

  double tennis_feet = LatLon::DistFeet(tennis1, tennis2);
  // Reports 78.204, which Earth agrees on.
  CHECK(tennis_feet > 78.1 && tennis_feet < 78.3) << tennis_feet;

  LatLon fort_pitt = LatLon::FromDegs(40.441543, -80.010314);
  LatLon washington_monument = LatLon::FromDegs(38.888094, -77.033911);

  double pgh_dc_miles = LatLon::DistMiles(fort_pitt,
                                          washington_monument);
  CHECK(pgh_dc_miles > 191.2 && pgh_dc_miles < 192.6) << pgh_dc_miles;
}

// This just tests that Gnomonic and InverseGnomonic are inverses.
static void TestGnomonic() {
  LatLon tennis1 = LatLon::FromDegs(40.442661, -79.941922);
  LatLon tennis2 = LatLon::FromDegs(40.442455, -79.942001);
  LatLon washington_monument = LatLon::FromDegs(38.888094, -77.033911);
  LatLon::Projection p = LatLon::Gnomonic(tennis1);
  LatLon::InverseProjection ip = LatLon::InverseGnomonic(tennis1);

  {
    auto [x, y] = p(tennis2);
    auto utennis2 = ip(x, y);
    double meters = LatLon::DistMeters(tennis2, utennis2);
    CHECK(meters < 0.1) << meters;
  }

  {
    auto [x, y] = p(washington_monument);
    auto uwm = ip(x, y);
    double meters = LatLon::DistMeters(washington_monument, uwm);
    CHECK(meters < 0.1) << meters;
  }
}

static void TestLinear() {
  // These are the coordinates of the tile from pactom.
  static constexpr double lat0 = 40.577355;
  static constexpr double lon0 = -80.183547;

  static constexpr double lat1 = 40.289646;
  static constexpr double lon1 = -79.516107;

  const LatLon tl = LatLon::FromDegs(lat0, lon0);
  const LatLon br = LatLon::FromDegs(lat1, lon1);

  static constexpr int tile_width = 11672;
  static constexpr int tile_height = 6596;

  LatLon::Projection p = LatLon::Linear(tl, br);
  LatLon::InverseProjection ip = LatLon::InverseLinear(tl, br);

  const LatLon fountain = LatLon::FromDegs(40.441807, -80.012831);
  const int fountain_x = 2985;
  const int fountain_y = 3106;

  {
    const auto [fx, fy] = p(fountain);
    double fxx = fx * tile_width;
    double fyy = fy * tile_height;

    double dx = (fxx - fountain_x);
    double dy = (fyy - fountain_y);

    double dist = sqrt(dx * dx + dy * dy);
    /*
    printf("Want %d,%d  got  %.3f,%.3f (dist %.4f)\n",
           fountain_x, fountain_y, fxx, fyy, dist);
    */
    // These coordinates are carefully tuned, so we should be very close.
    CHECK(dist < 5);

    const LatLon ll = ip(fx, fy);
    double meters = LatLon::DistMeters(fountain, ll);
    CHECK(meters < 0.1) << meters;
  }
}

int main(int argc, char **argv) {
  TestPlusMinusMod();
  TestParse();

  TestDistance();
  TestGnomonic();

  TestLinear();

  printf("OK\n");
  return 0;
}

