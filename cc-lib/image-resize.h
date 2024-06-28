
#ifndef _CC_LIB_IMAGE_RESIZE_H
#define _CC_LIB_IMAGE_RESIZE_H

#include "image.h"

struct ImageResize {
  static ImageRGBA Resize(const ImageRGBA &src, int w, int h);

  // TODO: Float
};

#endif
