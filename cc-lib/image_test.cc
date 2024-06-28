
#include "image.h"

#include <unordered_set>
#include <functional>
#include <cstdint>

#include "base/logging.h"
#include "arcfour.h"

using uint8 = uint8_t;
using uint32 = uint32_t;

static void TestCreateAndDestroy() {
  {
    ImageRGBA img(100, 20);
  }

  {
    ImageRGBA img;
  }

  {
    std::vector<uint8_t> pixels(8, 0);
    ImageRGBA img(pixels, 2, 1);
  }

  {
    ImageA imga(100, 200);
  }

  {
    ImageF imgf(5, 7);
  }

  {
    Image1 img1(333, 1);
  }

  {
    Image1 img1(3, 41);
  }

  {
    Image1 img1;
  }

  {
    std::vector<bool> pixels(20 * 5, false);
    Image1 img1(pixels, 20, 5);
  }

}

static ImageRGB RandomRGB(ArcFour *rc, int width, int height) {
  ImageRGB img(width, height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8 r = rc->Byte();
      uint8 g = rc->Byte();
      uint8 b = rc->Byte();
      img.SetPixel(x, y, r, g, b);
    }
  }
  return img;
}

// Overload std::hash to show how to put images in unordered_set, etc.
// TODO: Consider promoting this to the header; it's not that bad.
// (Overloading std:: functions is allowed if they involve user-defined
// types.)
template<>
struct std::hash<ImageRGBA> {
  std::size_t operator()(const ImageRGBA &s) const noexcept {
    return s.Hash();
  }
};

static void TestCopies() {
  ImageRGBA img(10, 20);
  img.Clear32(0x000000FF);
  img.SetPixel32(2, 3, 0xFF00FF77);
  img.SetPixel32(5, 4, 0xFF7700FF);

  ImageRGBA img2 = img;
  CHECK(img == img2);
  CHECK(img.GetPixel32(2, 3) == 0xFF00FF77);
  CHECK(img2.GetPixel32(2, 3) == 0xFF00FF77);

  CHECK(img.Red() == img2.Red());
  CHECK(img.Green() == img2.Green());
  CHECK(img.Blue() == img2.Blue());
  CHECK(img.Alpha() == img2.Alpha());

  img2.SetPixel32(2, 3, 0x112233FF);
  CHECK(img != img2);
  CHECK(img.GetPixel32(2, 3) == 0xFF00FF77);
  CHECK(img2.GetPixel32(2, 3) == 0x112233FF);

  CHECK(img.Red() != img2.Red());
  CHECK(img.Green() != img2.Green());
  CHECK(img.Blue() != img2.Blue());
  CHECK(img.Alpha() != img2.Alpha());

  CHECK(img.Hash() != img2.Hash()) << "Technically we can "
    "have hash collisions, but this would be pretty unlucky";

  {
    std::unordered_set<ImageRGBA> s;
    CHECK(!s.contains(img));
    s.insert(img);
    CHECK(!s.contains(img2));
    CHECK(s.contains(img));
    s.insert(img2);
    CHECK(s.size() == 2);
  }
}

static void TestBilinearResize() {
  ImageA in(20, 20);
  in.Clear(0);
  for (int i = 0; i < 16; i++) {
    in.SetPixel(i, i / 2, i * 15);
  }

  in.BlendText(1, 9, 0xCC, ":)");
  in.GreyscaleRGBA().Save("image-test-bilinear-resize-in.png");

  ImageRGBA big = in.ResizeBilinear(120, 120).GreyscaleRGBA();
  big.Save("image-test-bilinear-resize-out.png");
}

// XXX this just tests clipping; actually test the sampling!
static void TestSampleBilinear() {
  ImageF in(5, 5);
  in.Clear(1.0f);

  {
    // This sample is well outside the image, so it should
    // return the third arg.
    float out = in.SampleBilinear(10.0, 10.0, 0.25);
    CHECK(std::abs(out - 0.25f) < 0.00001f);
  }
}

static void TestEq() {
  ImageA one(10, 20);
  one.Clear(0x7F);
  ImageA two = one;
  CHECK(two == one);
  two.SetPixel(3, 9, 0x11);
  CHECK(!(two == one));
}

static void TestScaleDown() {
  ImageRGBA img(10, 10);
  img.Clear32(0x00FF0000);
  img.SetPixel32(0, 0, 0xFF000001);
  img.SetPixel32(1, 0, 0x0000FF0F);
  img.SetPixel32(0, 1, 0x00FF00AA);
  // max value pixel
  img.SetPixel32(2, 0, 0xFFFFFFFF);
  img.SetPixel32(3, 0, 0xFFFFFFFF);
  img.SetPixel32(2, 1, 0xFFFFFFFF);
  img.SetPixel32(3, 1, 0xFFFFFFFF);

  img.BlendText32(2, 2, 0xFFFFFFFF, ":)");
  img.BlendLine32(9, 0, 0, 9, 0x00FFFFCC);
  img.BlendLine32(0, 4, 9, 9, 0xFFFF00EE);
  img.Save("image-test-scaledown-in.png");
  ImageRGBA out = img.ScaleDownBy(2);
  out.Save("image-test-scaledown-out.png");

  CHECK(out.Width() == img.Width() / 2);
  CHECK(out.Height() == img.Height() / 2);
  {
    auto [r, g, b, a] = out.GetPixel(0, 0);
    CHECK(a == (0xAA + 0x0F + 0x01 + 0x00) / 4);
    CHECK(r < 0x04);
    CHECK(g > 0x7F);
    CHECK(b < 0x20);
  }

  uint32 white = out.GetPixel32(1, 0);
  CHECK(white == 0xFFFFFFFF) << white;
}

static void TestLineEndpoints() {
  ImageRGBA img(10, 10);
  img.Clear32(0x000000FF);
  img.BlendLine32(1, 1, 3, 3, 0xFFFFFFFF);
  CHECK(img.GetPixel32(1, 1) == 0xFFFFFFFF);
  CHECK(img.GetPixel32(3, 3) == 0xFFFFFFFF);

  // TODO: Test other directions of lines
}

static void TestFilledCircle() {
  {
    ImageRGBA img(10, 10);
    img.Clear32(0x000000FF);
    // This should cover the entire image.
    img.BlendFilledCircle32(4, 4, 10, 0x23458A77);
    for (int y = 0; y < 10; y++) {
      for (int x = 0; x < 10; x++) {
        uint32_t color = img.GetPixel32(x, y);
        CHECK(color == 0x102040FF) << color;
      }
    }
  }

  {
    ImageRGBA img(10, 10);
    img.Clear32(0x000000FF);
    // This should cover the entire image.
    img.BlendFilledCircle32(4, 4, 1, 0x23458A77);
    uint32_t in_color = img.GetPixel32(4, 4);
    CHECK(in_color == 0x102040FF) << in_color;

    uint32_t out_color1 = img.GetPixel32(5, 5);
    CHECK(out_color1 == 0x000000FF);
    uint32_t out_color2 = img.GetPixel32(3, 5);
    CHECK(out_color2 == 0x000000FF);
  }

  // TODO: More circle tests
}

static void TestCopyImage() {
  ImageRGBA brush(3, 2);
  brush.Clear32(0xFFFFFFFF);
  // Red in top left, blue in bottom right
  brush.SetPixel32(0, 0, 0xFF0000FF);
  brush.SetPixel32(2, 1, 0x0000FFFF);

  ImageRGBA canvas(10, 9);
  canvas.Clear32(0x666666FF);

  // Completely clipped.
  canvas.CopyImage(-1000, -1000, brush);
  canvas.CopyImage(1000, -1000, brush);
  canvas.CopyImage(-1000, 1000, brush);
  canvas.CopyImage(1000, 1000, brush);

  // Set top-left pixel blue
  canvas.CopyImage(-2, -1, brush);

  // And bottom-right pixel red
  canvas.CopyImage(9, 8, brush);

  // brush.ScaleBy(10).Save("deleteme-brush.png");
  // canvas.ScaleBy(10).Save("deleteme-canvas.png");

# define CHECK_COPYIMAGE() do {                      \
    for (int y = 0; y < 9; y++) {                    \
      for (int x = 0; x < 10; x++) {                 \
        uint32_t c = canvas.GetPixel32(x, y);        \
        if (0) printf("%08x %d,%d\n", c, x, y);      \
        if (y == 0 && x == 0) {                      \
          CHECK(c == 0x0000FFFF);                    \
        } else if (y == 8 && x == 9) {               \
          CHECK(c == 0xFF0000FF);                    \
        } else {                                     \
          CHECK(c == 0x666666FF) << x << " " << y;   \
        }                                            \
      }                                              \
    }                                                \
  } while (0)

  CHECK_COPYIMAGE();

  canvas.CopyImage(0, 0, canvas);

  CHECK_COPYIMAGE();

  // XXX test self-copies with overlap (this should
  // be correct now, but it is not really tested)
}

static void TestVerticalText() {
  ImageRGBA img(256, 128);
  img.Clear32(0x000000FF);

  img.BlendBox32(10, 10, img.Width() - 10 - 10, img.Height() - 10 - 10,
                 0x3333FFAA, 0x3333FF55);

  img.BlendTextVert32(11, 11, false, 0xFFFF00FF, "hey test");
  img.BlendTextVert32(img.Width() - 10 - 1 - 9,
                      img.Height() - 10 - 1,
                      true, 0xFF00FFFF, "what is this");
  img.Save("image-test-vertical-text.png");
}

static void TestConvertRGB() {
  ArcFour rc("convert");
  ImageRGB img24 = RandomRGB(&rc, 128, 100);

  ImageRGBA img32 = img24.AddAlpha();

  ImageRGB img24_b = img32.IgnoreAlpha();

  CHECK(img24 == img24_b);
}

static void TestSaveRGB() {
  ArcFour rc("save");
  ImageRGB img24 = RandomRGB(&rc, 128, 100);

  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 51; x++) {
      img24.SetPixel(x + 18, y + 6, x * 5, y * 4, 0x00);
    }
  }

  img24.SavePNG("image-test-rgb.png");
  img24.SaveJPG("image-test-rgb.jpg", 50);
}

static void TestRoundTripA1() {
  ArcFour rc("inv");

  static constexpr int W = 55, H = 41;
  ImageA imga(W, H);
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      imga.SetPixel(x, y, rc.Byte() > 128 ? 0xAA : 0x21);
    }
  }

  Image1 img1 = imga.Threshold(0x80);
  CHECK(img1.Width() == W);
  CHECK(img1.Height() == H);
  Image1 img1i = img1.Inverse();
  CHECK(!(img1 == img1i));
  CHECK(img1.Hash() != img1i.Hash());
  Image1 img1ii = img1i.Inverse();
  CHECK(img1 == img1ii);
  CHECK(img1.Hash() == img1ii.Hash());

  ImageA imgaii = img1ii.MonoA(0xAA, 0x21);
  CHECK(imgaii == imga);
  CHECK(imgaii.Hash() == imga.Hash());

  {
    Image1 white1(12, 13);
    white1.Clear(true);

    Image1 white2(12, 13);
    white2.Clear(false);
    white2 = white2.Inverse();

    CHECK(white1 == white2);
    CHECK(white1.Hash() == white2.Hash());
  }
}

int main(int argc, char **argv) {
  TestCreateAndDestroy();
  TestCopies();
  TestBilinearResize();
  TestSampleBilinear();
  TestEq();
  TestScaleDown();
  TestLineEndpoints();
  TestFilledCircle();
  TestCopyImage();

  TestVerticalText();

  TestRoundTripA1();

  TestConvertRGB();
  TestSaveRGB();

  // TODO: Test SaveToVec / LoadFromMemory round trip

  // TODO: More image tests!

  printf("OK\n");
  return 0;
}
