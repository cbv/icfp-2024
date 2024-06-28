
// Based on nv_voronoi.h, by Nicholas Vining, public domain; no warranty.
// (nicholas dot vining 'at' gas lamp games dot com)
//
// I think the original contains a bug where the rightmost column of
// the rasterization has the wrong values. In my wrapper I just generate
// an image that's one more pixel wide and discard it.

// This is a small implementation of the method for Voronoi cell
// generation, using the method described in "Linear Time Euclidean
// Distance Algorithms" by Breu et al. It only uses integer
// arithmetic, meaning that it should be deterministic. The paper
// itself is pretty vague on a bunch of things; in particular, it
// never explains how you actually *use* the list of voronoi centers
// that you are given to render the resulting image (answer: use the
// perpendicular bisector calculation.)
//
// You might want to use this if you are deterministically generating
// terrain over a network, or if you need Euclidean distance (from,
// say, obstacles.)

// TODO: Might be integer overflow for 4k-sized inputs?

#include "integer-voronoi.h"

#include <stdlib.h>
#include <cstdint>
#include <cmath>

#include "base/logging.h"
#include "image.h"

static int voronoi_perpendicular_bisector(
    int ux, int uy, int vx, int vy, int r) {
  return ((vx * vx) - (ux * ux) - (2 * r * (vy - uy)) + (vy * vy) - (uy * uy)) /
         (2 * (vx - ux));
}

static bool remove_candidate(int ux, int uy, int vx, int vy, int wx, int wy,
                             int r) {
  int a = ((wx - vx) * ((vx * vx) - (ux * ux) - (2 * r * (vy - uy)) +
                        (vy * vy) - (uy * uy)));
  int b = ((vx - ux) * ((wx * wx) - (vx * vx) - (2 * r * (wy - vy)) +
                        (wy * wy) - (vy * vy)));
  return a >= b;
}

static int compare_two_feature_points(const void *a, const void *b) {
  const int *x = (const int *)a;
  const int *y = (const int *)b;

  if (x[1] > y[1]) {
    return 1;
  } else if (x[1] < y[1]) {
    return -1;
  } else if (x[0] < y[0]) {
    return -1;
  } else if (x[0] > y[0]) {
    return 1;
  } else {
    return 0;
  }
}

static void build_candidates(int *candidates, int num_points,
                             int *feature_points, int w, int h, bool flipped) {
  for (int i = 0; i < w * h; i++) {
    candidates[i] = -1; // no candidate point found yet
  }

  if (flipped) {
    for (int i = 0; i < num_points; i++) {
      if (feature_points[i * 2 + 1] == h - 1) {
        candidates[(h - 1) * w + feature_points[i * 2 + 0]] = i;
      }
    }

    for (int j = h - 2; j >= 0; j--) {
      for (int i = 0; i < w; i++) {
        candidates[j * w + i] = candidates[(j + 1) * w + i];
      }
      for (int i = num_points - 1; i >= 0; i--) {
        if (feature_points[i * 2 + 1] == j) {
          candidates[j * w + feature_points[i * 2 + 0]] = i;
        }
        if (feature_points[i * 2 + 1] < j) {
          break; // STOP!
        }
      }
    }
  } else {
    for (int i = 0; i < num_points; i++) {
      if (feature_points[i * 2 + 1] == 0) {
        candidates[feature_points[i * 2 + 0]] = i;
      }
      if (feature_points[i * 2 + 1] > 0) {
        break;
      }
    }

    // build candidates

    for (int j = 1; j < h; j++) {
      for (int i = 0; i < w; i++) {
        candidates[j * w + i] = candidates[(j - 1) * w + i];
      }
      for (int i = 0; i < num_points; i++) {
        if (feature_points[i * 2 + 1] == j) {
          candidates[j * w + feature_points[i * 2 + 0]] = i;
        }
        if (feature_points[i * 2 + 1] > j) {
          break; // STOP!
        }
      }
    }
  }
}

static void possibly_write(int x, int y, int *old_val, int new_val,
                           int *feature_points) {
  if (*old_val == -1) {
    *old_val = new_val;
  } else {
    int old_x = feature_points[(*old_val) * 2 + 0] - x;
    int old_y = feature_points[(*old_val) * 2 + 1] - y;
    int new_x = feature_points[new_val * 2 + 0] - x;
    int new_y = feature_points[new_val * 2 + 1] - y;

    int old_dist = old_x * old_x + old_y * old_y;
    int new_dist = new_x * new_x + new_y * new_y;

    // Have a face-off. Which am I closer to?

    if (old_dist > new_dist) {
      *old_val = new_val;
    }
  }
}

static void inner_loop(int *candidates, int num_points, int *feature_points,
                       int *dest, int w, int h, bool flipped) {
  int *candidate_row = (int *)malloc(w * sizeof(int));
  int *l_row = (int *)malloc(w * sizeof(int));
  int *centers = (int *)malloc(w * sizeof(int));

  for (int i = flipped ? h - 1 : 0; flipped ? (i >= 0) : (i < h);
       (flipped ? i-- : i++)) {
    int target_i = i;

    // for each row, we compute:
    //
    // candidates - all points that COULD be in L_r
    // L_r - all points that are actual voronoi centers that intersect with the
    // row R

    int num_candidates = 0;
    int k = 1;
    int l = 2;

    for (int j = 0; j < w; j++) {
      if (candidates[i * w + j] != -1) {
        candidate_row[num_candidates] = candidates[i * w + j];
        num_candidates++;
      }
    }

    if (num_candidates == 0) {
      continue;
    }
    if (num_candidates == 1) {
      for (int j = 0; j < w; j++) {
        possibly_write(j, target_i, &dest[target_i * w + j], candidate_row[0],
                       feature_points);
      }
      continue;
    }

    if (num_candidates > 0) {
      l_row[0] = candidate_row[0];
    }
    if (num_candidates > 1) {
      l_row[1] = candidate_row[1];
    }

    if (num_candidates == 2) {
      for (int j = 0; j < feature_points[l_row[0] * 2 + 0]; j++) {
        possibly_write(j, target_i, &dest[target_i * w + j], l_row[0],
                       feature_points);
      }
      for (int j = feature_points[l_row[0] * 2 + 0];
           j < feature_points[l_row[1] * 2 + 0]; j++) {
        int distX1 = feature_points[l_row[0] * 2 + 0] - j;
        int distY1 = feature_points[l_row[0] * 2 + 1] - target_i;
        int distX2 = feature_points[l_row[1] * 2 + 0] - j;
        int distY2 = feature_points[l_row[1] * 2 + 1] - target_i;

        if (distX1 * distX1 + distY1 * distY1 <
            distX2 * distX2 + distY2 * distY2) {
          possibly_write(j, target_i, &dest[target_i * w + j], l_row[0],
                         feature_points);
        } else {
          possibly_write(j, target_i, &dest[target_i * w + j], l_row[1],
                         feature_points);
        }
      }
      for (int j = feature_points[l_row[1] * 2 + 0]; j < w; j++) {
        possibly_write(j, target_i, &dest[target_i * w + j], l_row[1],
                       feature_points);
      }
      continue;
    }

    // This is slightly different from the paper because they start their tables
    // numbered at 1; yech
    while (l < num_candidates) {
      int w = candidate_row[l];
      while (k >= 1 && remove_candidate(feature_points[l_row[k - 1] * 2 + 0],
                                        feature_points[l_row[k - 1] * 2 + 1],
                                        feature_points[l_row[k] * 2 + 0],
                                        feature_points[l_row[k] * 2 + 1],
                                        feature_points[w * 2 + 0],
                                        feature_points[w * 2 + 1], i)) {
        k = k - 1;
      }
      k = k + 1;
      l = l + 1;
      l_row[k] = w;
    }

    for (int j = 0; j < k; j++) {
      centers[j] = voronoi_perpendicular_bisector(
          feature_points[l_row[j] * 2 + 0], feature_points[l_row[j] * 2 + 1],
          feature_points[l_row[j + 1] * 2 + 0],
          feature_points[l_row[j + 1] * 2 + 1], i);
      if (centers[j] < 0) {
        centers[j] = 0;
      }
      if (centers[j] > w - 1) {
        centers[j] = w - 1;
      }
    }

    for (int j = 0; j < centers[0]; j++) {
      possibly_write(j, target_i, &dest[target_i * w + j], l_row[0],
                     feature_points);
    }

    for (int m = 0; m < k - 1; m++) {
      for (int j = centers[m]; j < centers[m + 1]; j++) {
        int distX1 = feature_points[l_row[m] * 2 + 0] - j;
        int distY1 = feature_points[l_row[m] * 2 + 1] - target_i;
        int distX2 = feature_points[l_row[m + 1] * 2 + 0] - j;
        int distY2 = feature_points[l_row[m + 1] * 2 + 1] - target_i;

        if (distX1 * distX1 + distY1 * distY1 <
            distX2 * distX2 + distY2 * distY2) {
          possibly_write(j, target_i, &dest[target_i * w + j], l_row[m],
                         feature_points);
        } else {
          possibly_write(j, target_i, &dest[target_i * w + j], l_row[m + 1],
                         feature_points);
        }
      }
    }

    for (int j = centers[k - 1]; j < w; j++) {
      possibly_write(j, target_i, &dest[target_i * w + j], l_row[k],
                     feature_points);
    }
  }

  free(candidate_row);
  free(l_row);
  free(centers);
}

// PERF: There are several places here where we are converting
// formats or copying.

// feature_points is an array of alternating x,y coordinates.
void linear_time_voronoi(int num_points, int *feature_points, int *dest, int w,
                         int h) {
  int *candidates;

  qsort(feature_points, num_points, sizeof(int) * 2,
        compare_two_feature_points);

  for (int i = 0; i < w * h; i++) {
    dest[i] = -1;
  }

  candidates = (int *)malloc(w * h * sizeof(int));

  build_candidates(candidates, num_points, feature_points, w, h, false);
  inner_loop(candidates, num_points, feature_points, dest, w, h, false);

  // build candidates

  build_candidates(candidates, num_points, feature_points, w, h, true);
  inner_loop(candidates, num_points, feature_points, dest, w, h, true);

  free(candidates);
}

// This generates the requested size using the original algorithm,
// but note that the last column is wrong.
static std::vector<int>
RasterizeVecInternal(const std::vector<std::pair<int, int>> &points,
                     int width, int height) {
  std::vector<int> flat_points(points.size() * 2, 0);
  for (int i = 0; i < (int)points.size(); i++) {
    const auto &[x, y] = points[i];
    flat_points[i * 2 + 0] = x;
    flat_points[i * 2 + 1] = y;
  }

  std::vector<int> dest(width * height, 0);
  linear_time_voronoi((int)points.size(), flat_points.data(),
                      dest.data(), width, height);

  return dest;
}

std::vector<int>
IntegerVoronoi::RasterizeVec(const std::vector<std::pair<int, int>> &points,
                             int width, int height) {
  // We generate one more pixel width, since the last one looks wrong.
  std::vector<int> raster = RasterizeVecInternal(points, width + 1, height);
  std::vector<int> out;
  out.reserve(width * height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      const int idx = y * (width + 1) + x;
      const int p = raster[idx];
      out.push_back(p);
    }
  }
  return out;
}

ImageRGBA
IntegerVoronoi::Rasterize32(const std::vector<std::pair<int, int>> &points,
                            int width, int height) {
  CHECK(points.size() < size_t{0x100000000});
  // As above, an extra column, discarded.
  std::vector<int> raster = RasterizeVecInternal(points, width + 1, height);
  ImageRGBA ret(width, height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      const int idx = y * (width + 1) + x;
      const int p = raster[idx];
      CHECK(p >= 0 && p < (int)points.size());
      ret.SetPixel32(x, y, (uint32_t)p);
    }
  }

  return ret;
}

ImageA IntegerVoronoi::Rasterize8(
    const std::vector<std::pair<int, int>> &points,
    int width, int height) {
  CHECK(points.size() < size_t{0x100});
  // As above, an extra column, discarded.
  std::vector<int> raster = RasterizeVecInternal(points, width + 1, height);
  ImageA ret(width, height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      const int idx = y * (width + 1) + x;
      const int p = raster[idx];
      CHECK(p >= 0 && p < (int)points.size());
      ret.SetPixel(x, y, (uint8_t)p);
    }
  }

  return ret;
}

ImageF IntegerVoronoi::DistanceField(const Image1 &img) {
  std::vector<std::pair<int, int>> points;
  for (int y = 0; y < img.Height(); y++) {
    for (int x = 0; x < img.Width(); x++) {
      if (img.GetPixel(x, y)) {
        points.emplace_back(x, y);
      }
    }
  }

  ImageRGBA vor = Rasterize32(points, img.Width(), img.Height());
  ImageF dist(img.Width(), img.Height());

  for (int y = 0; y < vor.Height(); y++) {
    for (int x = 0; x < vor.Width(); x++) {
      uint32_t p = vor.GetPixel32(x, y);
      CHECK(p < (uint32_t)points.size());

      const auto &[xx, yy] = points[p];

      int dx = xx - x;
      int dy = yy - y;
      int ddx = dx * dx;
      int ddy = dy * dy;

      float d = std::sqrt((float)(ddx + ddy));
      /*
      printf("%d,%d nearest %d= %d,%d; dist %.6f\n",
             x, y, p, xx, yy, d);
      */
      dist.SetPixel(x, y, d);
    }
  }

  return dist;
}

ImageF IntegerVoronoi::NormalizeDistanceField(const ImageF &img) {
  ImageF out(img.Width(), img.Height());
  if (img.Width() == 0 || img.Height() == 0) return out;

  float mxx = out.GetPixel(0, 0);
  float mnn = mxx;

  for (int y = 0; y < img.Height(); y++) {
    for (int x = 0; x < img.Width(); x++) {
      float p = img.GetPixel(x, y);
      // printf("%d,%d = %.6f\n", x, y, p);
      mxx = std::max(mxx, p);
      mnn = std::min(mnn, p);
    }
  }

  float span = mxx - mnn;
  float off = 0.0;
  float scale = 0.0;
  if (mnn <= mxx) {
    off = -mnn;
    scale = 1.0f / span;
  }

  for (int y = 0; y < img.Height(); y++) {
    for (int x = 0; x < img.Width(); x++) {
      float p = img.GetPixel(x, y);
      float np = (p + off) * scale;
      out.SetPixel(x, y, np);
      // printf("%d,%d. (%.6f + %.3f) * %.6f = %.6f\n", x, y, p, off, scale, np);
    }
  }

  return out;
}
