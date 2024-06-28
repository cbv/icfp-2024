#ifndef _CC_LIB_BASE64_H
#define _CC_LIB_BASE64_H

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

struct Base64 {
  static std::string Encode(std::string_view s);
  static std::string EncodeV(const std::vector<uint8_t> &v);
  /* XXX good if it could do error checking */
  static std::string Decode(std::string_view s);
  static std::vector<uint8_t> DecodeV(std::string_view s);

  // All characters in "a-zA-Z0-9+/" as well as =, which is used for padding.
  static bool IsBase64Char(char c);
};

#endif
