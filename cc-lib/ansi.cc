
#include "ansi.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <vector>
#include <utility>

#ifdef __MINGW32__
#include <windows.h>
#undef ARRAYSIZE
#endif

// TODO: Would be good to avoid this dependency too.
// We could use std::format in c++20, for example.
#include "base/stringprintf.h"
// For UTF8 decoding. Could easily duplicate it here.
#include "util.h"

using std::string;

void ANSI::Init() {
  #ifdef __MINGW32__
  // Turn on ANSI support in Windows 10+. (Otherwise, use ANSICON etc.)
  // https://docs.microsoft.com/en-us/windows/console/setconsolemode
  //
  // TODO: This works but seems to subsequently break control keys
  // and stuff like that in cygwin bash?
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
  // mingw headers may not know about this new flag
  static constexpr int kVirtualTerminalProcessing = 0x0004;
  DWORD old_mode = 0;
  GetConsoleMode(hStdOut, &old_mode);
  SetConsoleMode(hStdOut, old_mode | kVirtualTerminalProcessing);

  // XXX Not related to ANSI; enables utf-8 output.
  // Maybe we should separate this out? Someone might want to use
  // high-ascii output?
  SetConsoleOutputCP( CP_UTF8 );
  #endif
}

// TODO: It would be better if we had a way of stripping the
// terminal codes if they are not supported?
void CPrintf(const char* format, ...) {
  // Do formatting.
  va_list ap;
  va_start(ap, format);
  std::string result;
  StringAppendV(&result, format, ap);
  va_end(ap);

  #ifdef __MINGW32__
  DWORD n = 0;
  WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
               result.c_str(),
               result.size(),
               &n,
               nullptr);
  #else
  printf("%s", result.c_str());
  #endif
}

// Unpack 0xRRGGBB.
inline constexpr std::tuple<uint8_t, uint8_t, uint8_t>
Unpack24(uint32_t color) {
  return {(uint8_t)((color >> 16) & 255),
          (uint8_t)((color >> 8) & 255),
          (uint8_t)(color & 255)};
}

// and 0xRRGGBBAA
inline static constexpr std::tuple<uint8, uint8, uint8, uint8>
Unpack32(uint32 color) {
  return {(uint8)((color >> 24) & 255),
          (uint8)((color >> 16) & 255),
          (uint8)((color >> 8) & 255),
          (uint8)(color & 255)};
}

inline static constexpr uint32 Pack32(uint8 r, uint8 g, uint8 b, uint8 a) {
  return
    ((uint32)r << 24) | ((uint32)g << 16) | ((uint32)b << 8) | (uint32)a;
}

std::string ANSI::ForegroundRGB(uint8_t r, uint8_t g, uint8_t b) {
  char escape[24] = {};
  snprintf(escape, 24, "\x1B[38;2;%d;%d;%dm", r, g, b);
  return escape;
}

std::string ANSI::BackgroundRGB(uint8_t r, uint8_t g, uint8_t b) {
  // Max size is 12.
  char escape[24] = {};
  snprintf(escape, 24, "\x1B[48;2;%d;%d;%dm", r, g, b);
  return escape;
}

std::string ANSI::ForegroundRGB32(uint32_t rgba) {
  const auto [r, g, b, a_] = Unpack32(rgba);
  return ForegroundRGB(r, g, b);
}

std::string ANSI::BackgroundRGB32(uint32_t rgba) {
  const auto [r, g, b, a_] = Unpack32(rgba);
  return BackgroundRGB(r, g, b);
}

std::string ANSI::Time(double seconds) {
  char result[64] = {};
  if (seconds < 0.001) {
    snprintf(result, 64, AYELLOW("%.3f") "us", seconds * 1000000.0);
  } else if (seconds < 1.0) {
    snprintf(result, 64, AYELLOW("%.2f") "ms", seconds * 1000.0);
  } else if (seconds < 60.0) {
    snprintf(result, 64, AYELLOW("%.3f") "s", seconds);
  } else if (seconds < 60.0 * 60.0) {
    int sec = std::round(seconds);
    int omin = sec / 60;
    int osec = sec % 60;
    snprintf(result, 64, AYELLOW("%d") "m" AYELLOW("%02d") "s",
             omin, osec);
  } else {
    int sec = std::round(seconds);
    int ohour = sec / 3600;
    sec -= ohour * 3600;
    int omin = sec / 60;
    int osec = sec % 60;
    snprintf(result, 64, AYELLOW("%d") "h"
             AYELLOW("%d") "m"
             AYELLOW("%02d") "s",
             ohour, omin, osec);
  }
  return (string)result;
}

std::string ANSI::StripCodes(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  bool in_escape = false;
  for (int i = 0; i < (int)s.size(); i++) {
    if (in_escape) {
      if (s[i] >= '@' && s[i] <= '~') {
        in_escape = false;
      }
    } else {
      // OK to access one past the end of the string.
      if (s[i] == '\x1B' && s[i + 1] == '[') {
        in_escape = true;
        i++;
      } else {
        out += s[i];
      }
    }
  }
  return out;
}

int ANSI::StringWidth(const std::string &s) {
  // PERF could do this without copying.
  return StripCodes(s).size();
}


std::string ANSI::ProgressBar(uint64_t numer, uint64_t denom,
                              const std::string &operation,
                              double seconds,
                              ProgressBarOptions options) {
  double frac = numer / (double)denom;

  double spe = numer > 0 ? seconds / numer : 1.0;
  double remaining_sec = (denom - numer) * spe;
  string eta = Time(remaining_sec);
  int eta_len = StringWidth(eta);

  int bar_width = options.full_width - 2 - 1 - eta_len;
  // Number of characters that get background color.
  int filled_width = std::clamp((int)(bar_width * frac), 0, bar_width);
  string bar_text = StringPrintf("%llu / %llu  (%.1f%%) %s", numer, denom,
                                 frac * 100.0,
                                 operation.c_str());

  // TODO: Use CompositeRGBA for this.

  // could do "..."
  if ((int)bar_text.size() > bar_width) bar_text.resize(bar_width);
  bar_text.reserve(bar_width);
  while ((int)bar_text.size() < bar_width) bar_text.push_back(' ');

  // int unfilled_width = bar_width - filled_width;
  const auto &[fg_r, fg_g, fg_b] = Unpack24(options.fg);
  const auto &[bf_r, bf_g, bf_b] = Unpack24(options.bar_filled);
  const auto &[be_r, be_g, be_b] = Unpack24(options.bar_empty);
  string colored_bar =
    StringPrintf("%s"
                 // filled bar
                 "%s" "%s"
                 // empty bar
                 "%s" "%s"
                 ANSI_RESET,
                 ForegroundRGB(fg_r, fg_g, fg_b).c_str(),
                 BackgroundRGB(bf_r, bf_g, bf_b).c_str(),
                 bar_text.substr(0, filled_width).c_str(),
                 BackgroundRGB(be_r, be_g, be_b).c_str(),
                 bar_text.substr(filled_width).c_str());

  string out = AWHITE("[") + colored_bar + AWHITE("]") " " + eta;
  return out;
}

// From image.cc; see explanation there.
static inline uint32_t CompositeRGBA(uint32_t fg, uint32_t bg) {
  using word = uint16_t;
  const auto &[r, g, b, a] = Unpack32(fg);
  const auto &[old_r, old_g, old_b, old_a_] = Unpack32(bg);
  // so a + oma = 255.
  const word oma = 0xFF - a;
  const word rr = (((word)r * (word)a) + (old_r * oma)) / 0xFF;
  const word gg = (((word)g * (word)a) + (old_g * oma)) / 0xFF;
  const word bb = (((word)b * (word)a) + (old_b * oma)) / 0xFF;
  // Note that the components cannot be > 0xFF.
  if (rr > 0xFF) __builtin_unreachable();
  if (gg > 0xFF) __builtin_unreachable();
  if (bb > 0xFF) __builtin_unreachable();

  return Pack32(rr, gg, bb, 0xFF);
}

std::string ANSI::Composite(
      // ANSI codes are stripped.
      const std::string &text_raw,
      // entries are RGBA and character width. Alpha is composited.
      const std::vector<std::pair<uint32_t, int>> &fgcolors,
      // entries are RGBA and character width. Alpha is ignored.
      const std::vector<std::pair<uint32_t, int>> &bgcolors) {
  auto Width = [](const std::vector<std::pair<uint32_t, int>> &cv) {
      int w = 0;
      for (const auto &[c_, cw] : cv) w += cw;
      return w;
    };

  std::string text = StripCodes(text_raw);

  std::vector<uint32_t> codepoints = Util::UTF8Codepoints(text);

  int width = std::max(Width(fgcolors), Width(bgcolors));
  if (width <= 0) return "";
  if ((int)codepoints.size() > width) codepoints.resize(width);
  while ((int)codepoints.size() < width) codepoints.push_back(' ');

  // We could do this using the compact representation, but
  // since we're creating a string of this width anyway, we
  // would only be saving in constants.
  auto Flatten = [width](const std::vector<std::pair<uint32_t, int>> &cv) {
      std::vector<uint32_t> flat;
      flat.reserve(width);
      uint32_t last = 0;
      for (const auto &[c, w] : cv) {
        for (int i = 0; i < w; i++) flat.push_back(c);
        last = c;
      }

      while ((int)flat.size() < width) flat.push_back(last);
      return flat;
    };

  std::vector<uint32_t> fg = Flatten(fgcolors), bg = Flatten(bgcolors);

  // Now generate output string. Here we generate a color code whenever
  // the foreground or background actually changes.

  // This string will generally be longer than width because of the
  // ansi codes.
  std::string out;
  uint32_t last_fg = 0, last_bg = 0;
  for (int i = 0; i < width; i++) {
    uint32_t bgcolor = bg[i];
    uint32_t fgcolor = CompositeRGBA(fg[i], bgcolor);

    if (i == 0 || bgcolor != last_bg) {
      const auto &[r, g, b, a_] = Unpack32(bgcolor);
      StringAppendF(&out, "\x1B[48;2;%d;%d;%dm", r, g, b);
      last_bg = bgcolor;
    }

    if (i == 0 || fgcolor != last_fg) {
      const auto &[r, g, b, a_] = Unpack32(fgcolor);
      StringAppendF(&out, "\x1B[38;2;%d;%d;%dm", r, g, b);
      last_fg = fgcolor;
    }

    // And always add the text codepoint.
    out += Util::EncodeUTF8(codepoints[i]);
  }

  return out + ANSI_RESET;
}

