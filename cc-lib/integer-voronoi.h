
// For a set of points, computes in linear time its Voronoi diaram.
// The output is two-dimensional pixel grid (raster). Each pixel
// gets assigned the index of the point that it is closest to.

#ifndef _CC_LIB_INTEGER_VORONOI_H
#define _CC_LIB_INTEGER_VORONOI_H

#include <vector>
#include <utility>

#include "image.h"

struct IntegerVoronoi {
  // All of these functions are linear time.

  // Returns a vector of size width * height. Each pixel gives
  // the index of the input point that it is closest to.
  static std::vector<int>
  RasterizeVec(const std::vector<std::pair<int, int>> &points,
               int width, int height);

  // Compute the raster as pixels. Each pixel is a uint32_t giving
  // the index into the points vector, not an actual color value.
  static ImageRGBA Rasterize32(const std::vector<std::pair<int, int>> &points,
                               int width, int height);

  // Compute the raster as pixels. Requires that there are no more than
  // 256 points, since each "pixel" is represents the index of the
  // point from 0-255.
  static ImageA Rasterize8(const std::vector<std::pair<int, int>> &points,
                           int width, int height);

  // For a bitmap image, compute the distance from each point
  // to the nearest '1' pixel. The floats in the image give the
  // Euclidean distance, not color values. Linear time.
  static ImageF DistanceField(const Image1 &img);

  // Normalize so that the values are in [0, 1].
  static ImageF NormalizeDistanceField(const ImageF &img);
};


#endif
