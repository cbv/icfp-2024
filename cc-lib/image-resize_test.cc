#include "image-resize.h"

#include <string>
#include <memory>
#include <cstdio>
#include <algorithm>

#include "image.h"
#include "ansi.h"

static void ShowImageANSI(const ImageRGBA &img) {
  for (int y = 0; y < img.Height(); y++) {
    printf("  ");
    for (int x = 0; x < img.Width(); x++) {
      const auto &[r, g, b, a] = img.GetPixel(x, y);
      // composite on black.
      uint32_t rr = std::clamp(((uint32_t)r * (uint32_t)a) / 255, 0u, 255u);
      uint32_t gg = std::clamp(((uint32_t)g * (uint32_t)a) / 255, 0u, 255u);
      uint32_t bb = std::clamp(((uint32_t)b * (uint32_t)a) / 255, 0u, 255u);
      if (rr != 0 || gg != 0 || bb != 0) {
        printf("%s██" ANSI_RESET,
               ANSI::ForegroundRGB(rr, gg, bb).c_str());
      } else {
        printf("  ");
      }
    }
    printf("\n");
  }
}

static void TestSimple() {
  std::unique_ptr<ImageRGBA> favicon(ImageRGBA::Load("favicon.png"));
  ShowImageANSI(*favicon);

  ShowImageANSI(ImageResize::Resize(*favicon, 14, 14));
}

int main(int argc, char **argv) {
  TestSimple();
  return 0;
}
