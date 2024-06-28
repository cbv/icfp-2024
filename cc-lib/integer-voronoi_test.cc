#include "integer-voronoi.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <utility>
#include <unordered_set>

#include "image.h"
#include "arcfour.h"
#include "randutil.h"
#include "color-util.h"

#include "hashing.h"

#include "base/logging.h"
#include "base/stringprintf.h"

int main(int, char **) {
  ArcFour rc(StringPrintf("voronoi.%lld", time(nullptr)));

  int num_points = 256;
  int width = 1024;
  int height = 1024;

  std::vector<std::pair<int, int>> points;
  std::unordered_set<std::pair<int, int>, Hashing<std::pair<int, int>>>
    already;
  while ((int)points.size() < num_points) {
    const int x = RandTo(&rc, 1024);
    const int y = RandTo(&rc, 1024);
    if (!already.contains({x, y})) {
      points.emplace_back(x, y);
      already.insert({x, y});
    }
  }

  std::vector<int> raster =
    IntegerVoronoi::RasterizeVec(points, width, height);
  CHECK((int)raster.size() == width * height);

  std::vector<uint32_t> colors;
  for (int i = 0; i < (int)points.size(); i++) {
    float h = RandTo(&rc, 1024) / 1024.0f;
    float s = 0.5f + (RandTo(&rc, 512) / 1024.0f);
    float v = 0.45f + (RandTo(&rc, 512) / 1024.0f);
    const auto &[r, g, b] = ColorUtil::HSVToRGB(h, s, v);
    uint32_t c = ColorUtil::FloatsTo32(r, g, b, 1.0f);
    colors.push_back(c);
  }

  ImageRGBA vimg(width, height);

  for (int idx = 0; idx < (int)raster.size(); idx++) {
    int x = idx % width;
    int y = idx / width;

    int closest = raster[y * width + x];

    if (closest == -1) {
      // input point?
      vimg.SetPixel32(x, y, 0xFFFFFFFF);
    } else {
      CHECK(closest >= 0 && closest < (int)points.size());

      vimg.SetPixel32(x, y, colors[closest]);
    }
  }

  vimg.Save("voronoi-test.png");
  printf("Wrote voronoi-test.png\n");

  Image1 bitmap(width, height);
  bitmap.Clear();
  for (const auto &[x, y] : points) {
    bitmap.SetPixel(x, y, true);
  }

  ImageF dist = IntegerVoronoi::NormalizeDistanceField(
      IntegerVoronoi::DistanceField(bitmap));

  dist.Normalize();

  dist.Make8Bit().GreyscaleRGBA().Save("voronoi-test-dist.png");
  printf("Wrote voronoi-test-dist.png\n");

  printf("OK\n");
  return 0;
}
