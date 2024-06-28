
// TODO: Clean up for CC_LIB.

#ifndef _CC_LIB_ANSI_H
#define _CC_LIB_ANSI_H

#include <string>
#include <cstdint>
#include <vector>
#include <utility>

#define ANSI_PREVLINE "\x1B[F"
#define ANSI_CLEARLINE "\x1B[2K"
#define ANSI_CLEARTOEOL "\x1B[0K"
#define ANSI_BEGINNING_OF_LINE "\x1B[G"
// Put the cursor at the beginning of the current line, and clear it.
#define ANSI_RESTART_LINE ANSI_BEGINNING_OF_LINE ANSI_CLEARTOEOL
// Move to the previous line and clear it.
#define ANSI_UP ANSI_PREVLINE ANSI_CLEARLINE


#define ANSI_RED "\x1B[1;31;40m"
#define ANSI_GREY "\x1B[1;30;40m"
#define ANSI_BLUE "\x1B[1;34;40m"
#define ANSI_CYAN "\x1B[1;36;40m"
#define ANSI_YELLOW "\x1B[1;33;40m"
#define ANSI_GREEN "\x1B[1;32;40m"
#define ANSI_WHITE "\x1B[1;37;40m"
#define ANSI_PURPLE "\x1B[1;35;40m"
#define ANSI_RESET "\x1B[m"

#define ARED(s) ANSI_RED s ANSI_RESET
#define AGREY(s) ANSI_GREY s ANSI_RESET
#define ABLUE(s) ANSI_BLUE s ANSI_RESET
#define ACYAN(s) ANSI_CYAN s ANSI_RESET
#define AYELLOW(s) ANSI_YELLOW s ANSI_RESET
#define AGREEN(s) ANSI_GREEN s ANSI_RESET
#define AWHITE(s) ANSI_WHITE s ANSI_RESET
#define APURPLE(s) ANSI_PURPLE s ANSI_RESET

// standard two-level trick to expand and stringify
#define ANSI_INTERNAL_STR2(s) #s
#define ANSI_INTERNAL_STR(s) ANSI_INTERNAL_STR2(s)

// r, g, b must be numbers (numeric literals or #defined constants)
// in [0,255].
#define ANSI_FG(r, g, b) "\x1B[38;2;" \
  ANSI_INTERNAL_STR(r) ";" \
  ANSI_INTERNAL_STR(g) ";" \
  ANSI_INTERNAL_STR(b) "m"

#define ANSI_BG(r, g, b) "\x1B[48;2;" \
  ANSI_INTERNAL_STR(r) ";" \
  ANSI_INTERNAL_STR(g) ";" \
  ANSI_INTERNAL_STR(b) "m"

#define AFGCOLOR(r, g, b, s) "\x1B[38;2;" \
  ANSI_INTERNAL_STR(r) ";" \
  ANSI_INTERNAL_STR(g) ";" \
  ANSI_INTERNAL_STR(b) "m" s ANSI_RESET

#define ABGCOLOR(r, g, b, s) "\x1B[48;2;" \
  ANSI_INTERNAL_STR(r) ";" \
  ANSI_INTERNAL_STR(g) ";" \
  ANSI_INTERNAL_STR(b) "m" s ANSI_RESET

// non-standard colors. Perhaps should move some of these things
// to ansi-extended or something.
#define AORANGE(s) ANSI_FG(247, 155, 57) s ANSI_RESET

// Same as printf, but using WriteConsole on windows so that we
// can communicate with pseudoterminal. Without this, ansi escape
// codes will work (VirtualTerminalProcessing) but not mintty-
// specific ones. Calls normal printf on other platforms, which
// hopefully support the ansi codes.
void CPrintf(const char* format, ...);

namespace internal {
// This currently needs to be hoisted out due to a compiler bug.
// After it's fixed, put it back in the struct below.
struct ProgressBarOptions {
  // including [], time.
  int full_width = 76;
  // TODO: Maybe switch to RGBA and ignore alpha since we're now
  // using RGBA elsewhere.
  uint32_t fg = 0xfcfce6;
  uint32_t bar_filled = 0x0f1591;
  uint32_t bar_empty  = 0x00031a;
};
}

struct ANSI {

  // Call this in main for compatibility on windows.
  static void Init();

  // Without ANSI_RESET.
  static std::string ForegroundRGB(uint8_t r, uint8_t g, uint8_t b);
  static std::string BackgroundRGB(uint8_t r, uint8_t g, uint8_t b);
  // In RGBA format, but the alpha component is ignored.
  static std::string ForegroundRGB32(uint32_t rgba);
  static std::string BackgroundRGB32(uint32_t rgba);

  // Strip ANSI codes. Only CSI codes are supported (which is everything
  // in this file), and they are not validated. There are many nonstandard
  // codes that would be treated incorrectly here.
  static std::string StripCodes(const std::string &s);

  // Return the number of characters after stripping ANSI codes. Same
  // caveats as above.
  static int StringWidth(const std::string &s);

  // Return an ansi-colored representation of the duration, with arbitrary
  // but Tom 7-approved choices of color and significant figures.
  static std::string Time(double seconds);

  using ProgressBarOptions = internal::ProgressBarOptions;
  // Print a color progress bar. [numer/denom  operation] ETA HH:MM:SS
  // Doesn't update in place; you need to move the cursor with
  // like ANSI_UP.
  static std::string ProgressBar(uint64_t numer, uint64_t denom,
                                 // This currently can't have ANSI Codes,
                                 // because we need to split it into pieces.
                                 // TODO: Fix it!
                                 const std::string &operation,
                                 double taken,
                                 ProgressBarOptions options =
                                 ProgressBarOptions{});

  // Composites foreground and background text colors into an ansi
  // string.
  // Generates a fixed-width string using the fg/bgcolors arrays.
  // The length is the maximum of these two, with the last element
  // extended to fill. Text is truncated if it's too long.
  static std::string Composite(
      // ANSI codes are stripped.
      const std::string &text,
      // entries are RGBA and character width. Alpha is composited.
      const std::vector<std::pair<uint32_t, int>> &fgcolors,
      // entries are RGBA and character width. Alpha is ignored.
      const std::vector<std::pair<uint32_t, int>> &bgcolors);

  // TODO: A utility that strips the ansi codes and produces a
  // fgcolors array for them.

};

// Deprecated. Use ANSI:: versions.
inline std::string AnsiForegroundRGB(uint8_t r, uint8_t g, uint8_t b) {
  return ANSI::ForegroundRGB(r, g, b);
}
inline std::string AnsiBackgroundRGB(uint8_t r, uint8_t g, uint8_t b) {
  return ANSI::BackgroundRGB(r, g, b);
}

// Deprecated. Use ANSI::Init.
inline void AnsiInit() { return ANSI::Init(); }


#endif
