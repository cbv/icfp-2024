
#include "image-frgba.h"

#include <cstdio>
#include <cmath>

#include "base/logging.h"

#define CHECK_FEQ(f1, f2) do {                  \
  const float delta = fabs((f1) - (f2));        \
  CHECK(delta < 0.0001) << f1 << " vs " << f2   \
    << " got delta " << delta;                  \
  } while (false)

#define ASSERT_PIXEL(img, x, y, r, g, b, a) do {          \
  const auto [rr, gg, bb, aa] = (img).GetPixel((x), (y)); \
  CHECK_FEQ(rr, r);                                       \
  CHECK_FEQ(gg, g);                                       \
  CHECK_FEQ(bb, b);                                       \
  CHECK_FEQ(aa, a);                                       \
  } while (false)

static void Trivial() {
  ImageFRGBA img(2, 1);
  img.Clear(0.25, 0.75, 0.5, 1.0);
  ASSERT_PIXEL(img, 0, 0, 0.25, 0.75, 0.5, 1.0);
  img.SetPixel(0, 0, 0.1, 0.2, 0.3, 0.4);
  ASSERT_PIXEL(img, 0, 0, 0.1, 0.2, 0.3, 0.4);

  ImageFRGBA copy = img;
  ASSERT_PIXEL(copy, 1, 0, 0.25, 0.75, 0.5, 1.0);
  ASSERT_PIXEL(copy, 0, 0, 0.1, 0.2, 0.3, 0.4);
}

// XXX these tests depend on test files that are not checked
// in (they're huge) -- we should generate them with tinyexr
// on the fly instead
static void TestLoad() {
  printf("test-load\n");
  const char *FILENAME = "sample.exr";
  ImageFRGBA *fimg = ImageFRGBA::Load(FILENAME);
  CHECK(fimg != nullptr) << FILENAME << " (it's not checked in "
    "so you need to find it somewhere).";
  ImageRGBA img = fimg->ToRGBA();
  img.Save("image-frgba-test-sample.png");
  delete fimg;
  printf("done test-load\n");
}

static void TestLoadHuge() {
  printf("load-huge\n");
  const char *FILENAME = "starmap_2020_64k.exr";
  ImageFRGBA *fimg = ImageFRGBA::Load(FILENAME);
  CHECK(fimg != nullptr) << FILENAME << " (it's not checked in "
    "so you need to find it somewhere).";
  printf("Loaded %lld x %lld\n", fimg->Width(), fimg->Height());
  delete fimg;
}

int main(int argc, char **argv) {
  Trivial();

  TestLoad();

  TestLoadHuge();

  // XXX test something!

  printf("OK\n");
  return 0;
}
