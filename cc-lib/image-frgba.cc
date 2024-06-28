
#include "image-frgba.h"
#include "tinyexr.h"
#include "image.h"
#include "color-util.h"

#include "base/logging.h"
#include "util.h"

ImageFRGBA::ImageFRGBA(const std::vector<float> &rgbaf,
                       int64 width, int64 height) :
  width(width), height(height), rgba(rgbaf) {
  CHECK((int64)rgbaf.size() == width * height * 4);
}

ImageFRGBA::ImageFRGBA(const float *rgbaf,
                       int64 width, int64 height) :
  width(width), height(height) {
  printf("%lld x %lld x 4 = %lld\n", width, height, width * height * 4);
  rgba.resize(width * height * 4);
  for (int64 idx = 0; idx < width * height * 4; idx++) {
    rgba[idx] = rgbaf[idx];
  }
  printf("hi\n");
}

ImageFRGBA::ImageFRGBA(int64 width, int64 height) :
  width(width), height(height), rgba(width * height * 4, 0.0f) {
}

bool ImageFRGBA::operator==(const ImageFRGBA &other) const {
  return other.Width() == Width() &&
    other.Height() == Height() &&
    other.rgba == rgba;
}

void ImageFRGBA::Clear(float r, float g, float b, float a) {
  for (int64 px = 0; px < width * height; px++) {
    rgba[px * 4 + 0] = r;
    rgba[px * 4 + 1] = g;
    rgba[px * 4 + 2] = b;
    rgba[px * 4 + 3] = a;
  }
}

ImageRGBA ImageFRGBA::ToRGBA() const {
  ImageRGBA out(Width(), Height());
  for (int64 y = 0; y < Height(); y++) {
    for (int64 x = 0; x < Width(); x++) {
      const auto [r, g, b, a] = GetPixel(x, y);
      uint32_t c = ColorUtil::FloatsTo32(r, g, b, a);
      out.SetPixel32(x, y, c);
    }
  }
  return out;
}

ImageFRGBA::ImageFRGBA(const ImageRGBA &other) :
  ImageFRGBA(other.Width(), other.Height()) {
  for (int64 y = 0; y < Height(); y++) {
    for (int64 x = 0; x < Width(); x++) {
      uint32_t c = other.GetPixel32(x, y);
      const auto [r, g, b, a] = ColorUtil::U32ToFloats(c);
      SetPixel(x, y, r, g, b, a);
    }
  }
}

ImageFRGBA *ImageFRGBA::Load(const std::string &filename) {
  std::vector<uint8_t> buffer =
    Util::ReadFileBytes(filename);
  if (buffer.empty()) return nullptr;
  return LoadFromMemory(buffer);
}

ImageFRGBA *ImageFRGBA::LoadFromMemory(const std::vector<uint8_t> &buffer) {
  return LoadFromMemory((const unsigned char *)buffer.data(), buffer.size());
}

ImageFRGBA *ImageFRGBA::LoadFromMemory(const unsigned char *data, size_t size) {
  float *out_rgba = nullptr;
  int width = 0, height = 0;
  const char *err = nullptr;

  if (LoadEXRWithLayerFromMemory(
          &out_rgba, &width, &height, data, size, nullptr, &err) < 0) {
    if (err != nullptr) printf("Err: %s\n", err);
    return nullptr;
  }

  printf("%d x %d\n", width, height);

  if (width <= 0 || height <= 0) {
    if (err != nullptr) printf("Err: %s\n", err);
    return nullptr;
  }

  ImageFRGBA *ret = new ImageFRGBA(out_rgba, width, height);
  printf("ok?\n");
  free(out_rgba);
  printf("freeed\n");

  return ret;
}

std::tuple<float, float, float, float>
ImageFRGBA::SampleBilinear(double x, double y) const {
  // Truncate to integer pixels.
  int64 ix = x;
  int64 iy = y;

  // subpixel values give us the interpolants
  float fx = x - ix;
  float fy = y - iy;

  // Get these four values.
  //
  //  v00 ----- v10
  //   |   :fy   |
  //   |...*     | 1.0
  //   | fx      |
  //  v01 ----- v11
  //       1.0

  auto BClipPixel = [this](int64 x, int64 y) {
      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (x >= width) x = width - 1;
      if (y >= height) y = height - 1;
      return GetPixel(x, y);
    };

  using rgbaf = std::tuple<float, float, float, float>;

  rgbaf v00 = BClipPixel(ix, iy);
  rgbaf v10 = BClipPixel(ix + 1, iy);
  rgbaf v01 = BClipPixel(ix, iy + 1);
  rgbaf v11 = BClipPixel(ix + 1, iy + 1);

  auto Component = [fx, fy](
      float c00, float c10, float c01, float c11) -> float {
      // c0 interpolates between c00 and c10 at fx.
      float c0 = (float)c00 + (float)(c10 - c00) * fx;
      float c1 = (float)c01 + (float)(c11 - c01) * fx;

      float c = c0 + (c1 - c0) * fy;
      return c;
    };

  // TODO: Don't just average alpha; the average of #FF0000FF and
  // #00000000 should not be dark red.
  return std::make_tuple(
      // R
      Component(std::get<0>(v00), std::get<0>(v10),
                std::get<0>(v01), std::get<0>(v11)),
      // G
      Component(std::get<1>(v00), std::get<1>(v10),
                std::get<1>(v01), std::get<1>(v11)),
      // B
      Component(std::get<2>(v00), std::get<2>(v10),
                std::get<2>(v01), std::get<2>(v11)),
      // A
      Component(std::get<3>(v00), std::get<3>(v10),
                std::get<3>(v01), std::get<3>(v11)));
}

ImageFRGBA ImageFRGBA::Crop(int64 x, int64 y, int64 w, int64 h,
                            float r, float g, float b, float a) const {
  CHECK(w > 0 && h > 0) << w << " " << h;

  ImageFRGBA ret{w, h};
  // xx,yy in new image's coordinates
  for (int64 yy = 0; yy < h; yy++) {
    const int64 sy = yy + y;
    for (int64 xx = 0; xx < w; xx++) {
      const int64 sx = xx + x;
      // Fill color if not in bounds
      float sr = r, sg = g, sb = b, sa = a;
      if (sx >= 0 && sx < width &&
          sy >= 0 && sy < height) {
        std::tie(sr, sg, sb, sa) = GetPixel(sx, sy);
      }

      ret.SetPixel(xx, yy, sr, sg, sb, sa);
    }
  }
  return ret;
}

ImageFRGBA ImageFRGBA::ScaleBy(int scale) const {
  // 1 is not useful, but it does work
  CHECK(scale >= 1);
  ImageFRGBA ret(width * scale, height * scale);
  for (int64 y = 0; y < height; y++) {
    for (int64 x = 0; x < width; x++) {
      const auto [r, g, b, a] = GetPixel(x, y);
      for (int64 yy = 0; yy < scale; yy++) {
        for (int64 xx = 0; xx < scale; xx++) {
          ret.SetPixel(x * scale + xx,
                       y * scale + yy,
                       r, g, b, a);
        }
      }
    }
  }
  return ret;
}

ImageFRGBA ImageFRGBA::ScaleDownBy(int scale) const {
  // 1 is not useful, but it does work
  CHECK(scale >= 1);
  const int64 ww = width / scale;
  const int64 hh = height / scale;
  ImageFRGBA ret(ww, hh);
  for (int64 y = 0; y < hh; y++) {
    for (int64 x = 0; x < ww; x++) {
      float rr = 0.0f, gg = 0.0f, bb = 0.0f, aa = 0.0f;
      for (int yy = 0; yy < scale; yy++) {
        for (int xx = 0; xx < scale; xx++) {
          const auto [r, g, b, a] = GetPixel(x * scale + xx,
                                             y * scale + yy);

          // color contributions are alpha-weighted
          rr += r * a;
          gg += g * a;
          bb += b * a;
          aa += a;
        }
      }

      // Otherwise, the color can be anything, but output black.
      if (aa > 0.0f) {
        rr /= aa;
        gg /= aa;
        bb /= aa;
        aa /= scale * scale;
      }
      ret.SetPixel(x, y, rr, gg, bb, aa);
    }
  }
  return ret;
}
