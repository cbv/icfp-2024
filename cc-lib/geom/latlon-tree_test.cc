
#include "latlon-tree.h"

#include "geom/latlon.h"
#include "base/logging.h"
#include "arcfour.h"
#include "randutil.h"

using namespace std;

static void Instantiate() {
  internal::LLKDTree<int> llkd_int;
}

static void InsertApp() {
  ArcFour rc("insert-app");
  vector<pair<LatLon, bool>> points;

  LatLonTree<int> latlontree;

  // Insert it with its data being its index
  // in the points vector.
  for (int i = 0; i < 10000; i++) {
    LatLon ll = LatLon::FromDegs(
        // Avoid poles
        0.0 + RandDouble(&rc) * 50.0,
        -180.0 + RandDouble(&rc) * 360.0);
    latlontree.Insert(ll, (int)points.size());
    points.emplace_back(ll, false);
  }

  printf("Insert ok\n");

  // Find each node exactly once.
  latlontree.App([&points](LatLon ll, int idx) {
      CHECK(idx >= 0 && idx < points.size());
      CHECK(!points[idx].second) << idx;
      points[idx].second = true;
    });

  for (const auto &[ll, found] : points) {
    CHECK(found);
  }

  for (auto &p : points) p.second = false;

  printf("Applied to all points ok\n");

  // Find every point by radius search with a huge
  // radius.
  vector<tuple<LatLon, int, double>> res =
    latlontree.Lookup(
        LatLon::FromDegs(40.0, -80.0),
        // 25000 miles
        40233600.0);

  for (const auto &[ll, idx, dist_] : res) {
    CHECK(idx >= 0 && idx < points.size());
    CHECK(!points[idx].second) << idx;
    points[idx].second = true;
  }

  for (const auto &[ll, found] : points) {
    CHECK(found);
  }

  printf("Universal radius search ok\n");
}

static void AcrossMeridians() {
  LatLonTree<string> llt;

  LatLon a = LatLon::FromDegs(19.00, -179.99);
  LatLon b = LatLon::FromDegs(19.01, 179.99);
  llt.Insert(a, "a");
  llt.Insert(b, "b");

  auto CheckHasBoth = [](const auto &v) {
      CHECK(v.size() == 2);
      CHECK((std::get<1>(v[0]) == "a" && std::get<1>(v[1]) == "b") ||
            (std::get<1>(v[0]) == "b" && std::get<1>(v[1]) == "a"));
    };

  CheckHasBoth(llt.Lookup(a, 10000.0));
  CheckHasBoth(llt.Lookup(b, 10000.0));

  LatLon c = LatLon::FromDegs(19.00, -0.01);
  LatLon d = LatLon::FromDegs(19.01, 0.01);
  llt.Insert(c, "c");
  llt.Insert(d, "d");

  // Shouldn't affect other lookup
  CheckHasBoth(llt.Lookup(a, 10000.0));
  CheckHasBoth(llt.Lookup(b, 10000.0));

  auto CheckHasBoth2 = [](const auto &v) {
      CHECK(v.size() == 2);
      CHECK((std::get<1>(v[0]) == "c" && std::get<1>(v[1]) == "d") ||
            (std::get<1>(v[0]) == "d" && std::get<1>(v[1]) == "c"));
    };

  CheckHasBoth2(llt.Lookup(c, 10000.0));
  CheckHasBoth2(llt.Lookup(d, 10000.0));

  LatLon ny = LatLon::FromDegs(40.53, -74.16);
  // Bizarro New York in Dubai
  LatLon bny = LatLon::FromDegs(25.211949, 55.1544404);

  llt.Insert(ny, "ny");
  llt.Insert(bny, "bny");

  // Old lookups unaffected; these points are not within 10km
  CheckHasBoth(llt.Lookup(a, 10000.0));
  CheckHasBoth(llt.Lookup(b, 10000.0));
  CheckHasBoth2(llt.Lookup(c, 10000.0));
  CheckHasBoth2(llt.Lookup(d, 10000.0));

  auto v1 = llt.Lookup(ny, 10000.0);
  CHECK(v1.size() == 1 && std::get<1>(v1[0]) == "ny");
  auto v2 = llt.Lookup(bny, 10000.0);
  CHECK(v2.size() == 1 && std::get<1>(v2[0]) == "bny");

  printf("Across meridians ok\n");
}

int main(int argc, char **argv) {
  Instantiate();
  AcrossMeridians();

  InsertApp();


  printf("OK\n");

  return 0;
}
