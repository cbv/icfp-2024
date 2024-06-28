
#ifndef _CC_LIB_WAVESAVE_H
#define _CC_LIB_WAVESAVE_H

#include <string>
#include <vector>
#include <utility>
#include <cstdint>

struct WaveSave {

  // Samples should be in [-1, 1] or else bad clipping will happen.
  // Stereo, saved as 16-bit.
  static bool SaveStereo(const std::string &filename,
                         const std::vector<std::pair<float, float>> &samples,
                         int samples_per_sec);

  // Mono, saved as 16-bit.
  static bool SaveMono(const std::string &filename,
                       const std::vector<float> &samples,
                       int samples_per_sec);

  // Unsigned 16-bit samples, mono.
  static bool SaveMono16(const std::string &filename,
                         const std::vector<uint16_t> &samples,
                         int samples_per_sec);

  // Utilities:
  static void HardClipMono(std::vector<float> *samples,
                           float max_mag = 32766.0f / 32768.0f);
  static void HardClipStereo(std::vector<std::pair<float, float>> *samples,
                             float max_mag = 32766.0f / 32768.0f);

};

#endif
