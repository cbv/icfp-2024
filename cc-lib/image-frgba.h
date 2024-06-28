
// Wrapper for 4-channel float images, EXR loading/saving, etc.
// This is a somewhat more heavyweight library than image.h, so
// we keep the dependencies one-way.

#ifndef _CC_LIB_IMAGE_FRGBA_H
#define _CC_LIB_IMAGE_FRGBA_H

#include <string>
#include <vector>
#include <cstdint>
#include <tuple>

// For conversion to/from ImageRGBA and other basic formats.
#include "image.h"

// 4-channel image in R-G-B-A order. Each channel is float.
struct ImageFRGBA {
  using int64 = int64_t;
  ImageFRGBA(const std::vector<float> &rgbaf, int64 width, int64 height);
  ImageFRGBA(const float *rgbaf, int64 width, int64 height);
  ImageFRGBA(int64 width, int64 height);
  ImageFRGBA() : width(0), height(0) {}

  // Value semantics.
  ImageFRGBA(const ImageFRGBA &other) = default;
  ImageFRGBA(ImageFRGBA &&other) = default;
  ImageFRGBA &operator =(const ImageFRGBA &other) = default;
  ImageFRGBA &operator =(ImageFRGBA &&other) = default;

  // Conversion to/from 8-bit RGBA image.
  ImageRGBA ToRGBA() const;
  explicit ImageFRGBA(const ImageRGBA &other);

  // Requires exact equality of RGBA values (even if alpha is zero).
  bool operator ==(const ImageFRGBA &other) const;

  int64 Width() const { return width; }
  int64 Height() const { return height; }

  // Loads EXR format.
  static ImageFRGBA *Load(const std::string &filename);
  static ImageFRGBA *LoadFromMemory(const std::vector<uint8_t> &bytes);
  static ImageFRGBA *LoadFromMemory(const unsigned char *data, size_t size);

  // TODO: Saves EXR format.
  /*
  bool Save(const std::string &filename) const;
  std::vector<uint8_t> SaveToVec() const;
  std::string SaveToString() const;
  */

  // In RGBA order. Pixels outside the image are 0,0,0,0.
  inline std::tuple<float, float, float, float>
  GetPixel(int64 x, int64 y) const;

  // Clear the image to a single value.
  void Clear(float r, float g, float b, float a);

  inline void SetPixel(int64 x, int64 y,
                       float r, float g, float b, float a);

  // Crop (or pad), returning a new image of the given width and height.
  // If this includes any area outside the input image, fill with
  // fill_color.
  ImageFRGBA Crop(
      int64 x, int64 y, int64 w, int64 h,
      float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f) const;

  // Scale by a positive integer factor, crisp pixels.
  ImageFRGBA ScaleBy(int scale) const;
  // Scale down by averaging boxes of size scale x scale to produce
  // a pixel value. If the width and height are not divisible by
  // the scale, pixels are dropped.
  ImageFRGBA ScaleDownBy(int scale) const;

  // Blend pixel with existing data.
  // Note: Currently assumes existing alpha is 0xFF.
  void BlendPixel(int64 x, int64 y, float r, float g, float b, float a);

  // TODO: Utilities from Image.

  // Output value is RGBA floats (nominally) in [0, 1].
  // Alpha values are just treated as a fourth channel and averaged,
  // which is not really correct.
  // Treats the input pixels as being "located" at their top-left
  // corners (not their centers).
  // x/y out of bounds will repeat edge pixels.
  std::tuple<float, float, float, float>
  SampleBilinear(double x, double y) const;

  // Extract single channel.
  ImageF Red() const;
  ImageF Green() const;
  ImageF Blue() const;
  ImageF Alpha() const;

  // Images must all be the same dimensions.
  static ImageFRGBA FromChannels(const ImageF &red,
                                 const ImageF &green,
                                 const ImageF &blue,
                                 const ImageF &alpha);

private:
  int64 width, height;
  // Size width * height * 4.
  std::vector<float> rgba;
};



// Implementations follow.

std::tuple<float, float, float, float>
ImageFRGBA::GetPixel(int64 x, int64 y) const {
  // Treat out-of-bounds reads as containing 0,0,0,0.
  if ((uint64_t)x >= (uint64_t)width ||
      (uint64_t)y >= (uint64_t)height)
    return std::make_tuple(0.0f, 0.0f, 0.0f, 0.0f);

  const int64_t idx = (y * width + x) * 4;
  return std::make_tuple(
      rgba[idx + 0],
      rgba[idx + 1],
      rgba[idx + 2],
      rgba[idx + 3]);
}

void ImageFRGBA::SetPixel(int64 x, int64 y,
                          float r, float g, float b, float a) {
  if ((uint64_t)x >= (uint64_t)width) return;
  if ((uint64_t)y >= (uint64_t)height) return;

  const int64_t idx = (y * width + x) * 4;
  rgba[idx + 0] = r;
  rgba[idx + 1] = g;
  rgba[idx + 2] = b;
  rgba[idx + 3] = a;
}

#endif
