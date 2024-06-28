
#ifndef _CC_LIB_MP3_H
#define _CC_LIB_MP3_H

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

struct MP3 {

  static bool IsMP3(const std::string &filename);
  static bool IsMP3(const std::vector<uint8_t> &bytes);

  // Empty MP3; use static routines to create.
  MP3() {}

  static std::unique_ptr<MP3> Load(const std::string &filename);
  static std::unique_ptr<MP3> Decode(const std::vector<uint8_t> &bytes);

  int sample_rate_hz = 44100;
  int channels = 1;
  // Channels are interleaved.
  // Float samples are nominally in [-1, 1].
  std::vector<float> samples;

  // Information from encoding; can be safely ignored.
  int layer = 3;
  int bitrate_kbps = 128;

  // TODO: Would be nice to read ID3 tags for title/artist/etc.

};

#endif
