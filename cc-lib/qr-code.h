
#ifndef _CC_LIB_QR_CODE_H
#define _CC_LIB_QR_CODE_H

#include "image.h"

struct QRCode {
  // TODO: ECC settings, border, etc.
  // The returned image uses 0xFF for white (background) and
  // 0x00 for black (foreground).
  static ImageA Text(const std::string &str);

  // Adds a white border. This can be useful if displaying on a dark
  // background.
  static ImageA AddBorder(const ImageA &qr, int pixels);

 private:
  // All static.
  QRCode() = delete;
};

#endif
