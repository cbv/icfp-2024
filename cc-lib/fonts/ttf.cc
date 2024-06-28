#include "fonts/ttf.h"

#include <string>
#include <vector>
#include <optional>
#include <tuple>
#include <unordered_map>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "stb_truetype.h"

using namespace std;

TTF::TTF(const string &filename) {
  ttf_bytes = Util::ReadFileBytes(filename);
  CHECK(!ttf_bytes.empty()) << filename;

  int offset = stbtt_GetFontOffsetForIndex(ttf_bytes.data(), 0);
  CHECK(offset != -1);
  CHECK(stbtt_InitFont(&font, ttf_bytes.data(), offset)) <<
    "Failed to load " << filename;
  stbtt_GetFontVMetrics(
      &font, &native_ascent, &native_descent, &native_linegap);

  // We use a normalized representation like this:
  //
  //      0,0
  //      +------------------------ ascent = 0.0
  //      |
  //      |
  //      |      ,---,
  //      |     |     |
  //      |     `.___.`
  //      |
  //      |       ___
  //      |      /   |
  //      |      `.  |
  //      |       |  |
  //      |       |  |
  //      |       |  |
  //      |       |  |
  //      +-------+--+------------- baseline = 0.75
  //      |       |  |
  //      |      /   ;
  //      |     /   /
  //      |    /   /
  //     -+---`   /
  //    | |      /
  //    `-+------  ---------------- descent = 1.0
  //
  // Note, y-axis is flipped.
  //
  // Note the coordinates can be negative or greater than 1.0, but
  // they are "nominally" in [0,1] vertically. It is normal for
  // horizontal coordinates to be significantly larger than 1.

  // Note that native_descent is frequently wrong in fonts (it is positive).
  // I don't try to fix that here.

  int height = native_ascent - native_descent;
  norm = 1.0f / height;
  baseline = native_ascent * norm;

  /*
  printf("native ascent %d descent %d linegap %d.\n"
         "norm = %.5f  baseline = %.5f   lineheight %.5f\n",
         native_ascent, native_descent, native_linegap,
         norm, baseline, NormLineHeight());
  */

  #if 0
  int os2ascent, os2descent, os2lg;
  if (stbtt_GetFontVMetricsOS2(&font, &os2ascent, &os2descent, &os2lg)) {
    printf("OS/2 versions: %d %d %d\n", os2ascent, os2descent, os2lg);
  }
  #endif
}


std::pair<int, int>
TTF::MeasureString(const string &text, int size_px, bool subpixel) const {
  const float scale = stbtt_ScaleForPixelHeight(&font, size_px);

  int ascent = 0, descent = 0, line_gap = 0;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);

  float xpos = 0.0f;
  for (int idx = 0; idx < (int)text.size(); idx++) {

    int advance = 0, left_side_bearing = 0;
    stbtt_GetCodepointHMetrics(
        &font, text[idx], &advance, &left_side_bearing);

    xpos += advance * scale;
    if (text[idx + 1] != '\0') {
      xpos += scale *
        stbtt_GetCodepointKernAdvance(&font, text[idx], text[idx + 1]);
    }

    if (!subpixel) {
      // Or floor?
      xpos = roundf(xpos);
    }
  }
  return {xpos, ascent - descent + line_gap};
}


std::vector<TTF::Contour> TTF::GetContours(int codepoint) const {
  std::vector<NativeContour> ncs = GetNativeContours(codepoint);
  std::vector<Contour> out;
  out.reserve(ncs.size());
  for (const NativeContour &nc : ncs) {
    Contour c;
    c.paths.reserve(nc.paths.size());
    for (const NativePath &np : nc.paths) {
      switch (np.type) {
      case PathType::LINE: {
        const auto [x, y] = Norm(np.x, np.y);
        c.paths.emplace_back(x, y);
        break;
      }
      case PathType::BEZIER: {
        const auto [x, y] = Norm(np.x, np.y);
        const auto [cx, cy] = Norm(np.cx, np.cy);
        c.paths.emplace_back(x, y, cx, cy);
        break;
      }
      }
    }

    const auto [sx, sy] = Norm(nc.StartX(), nc.StartY());
    CHECK_EQ(c.StartX(), sx);
    CHECK_EQ(c.StartY(), sy);
    out.emplace_back(std::move(c));
  }
  return out;
}


std::vector<TTF::Contour> TTF::MakeOnlyBezier(
    const std::vector<Contour> &contours) {
  std::vector<Contour> out;
  out.reserve(contours.size());
  for (const Contour &c : contours) {
    Contour o;
    float x = c.StartX(), y = c.StartY();
    for (const Path &p : c.paths) {
      switch (p.type) {
      case PathType::LINE: {
        // This is equivalent to a Bezier curve where the control
        // point lies (anywhere) on the line segment. The nicest
        // choice seems to be the midpoint.
        float mx = (p.x + x) * 0.5f;
        float my = (p.y + y) * 0.5f;
        o.paths.emplace_back(p.x, p.y, mx, my);
        x = p.x;
        y = p.y;
        break;
      }
      case PathType::BEZIER:
        o.paths.emplace_back(p.x, p.y, p.cx, p.cy);
        x = p.x;
        y = p.y;
        break;
      }
    }
    out.emplace_back(std::move(o));
  }
  return out;
}

std::vector<TTF::Contour> TTF::NormalizeOrder(
    const std::vector<Contour> &contours,
    float origin_x, float origin_y) {
  auto SqDist = [origin_x, origin_y](float x, float y) {
      float dx = (x - origin_x);
      float dy = (y - origin_y);
      return (dx * dx) + (dy * dy);
    };
  std::vector<Contour> out;
  out.reserve(contours.size());
  for (const Contour &c : contours) {
    // index (of end point) closest to the origin point.
    // -1 means the start point was already closest.
    int bestidx = -1;
    float best_sqerr = SqDist(c.StartX(), c.StartY());
    for (int i = 0; i < (int)c.paths.size(); i++) {
      // Both LINE and BEZIER have an end point.
      const float sqerr = SqDist(c.paths[i].x, c.paths[i].y);
      if (sqerr < best_sqerr) {
        bestidx = i;
        best_sqerr = sqerr;
      }
    }

    if (bestidx == -1) {
      out.push_back(c);
    } else {
      // Some end point was closer than the current start.
      // Start there instead.
      float x = c.paths[bestidx].x;
      float y = c.paths[bestidx].y;
      Contour r;
      // Paths that come after it.
      for (int i = bestidx + 1; i < (int)c.paths.size(); i++) {
        r.paths.push_back(c.paths[i]);
        x = c.paths[i].x;
        y = c.paths[i].y;
      }
      // When we get to the end, we assume a closed path.
      // (If not, we could always insert a LINE here.
      CHECK_EQ(c.StartX(), x) << c.StartX() << " " << x;
      CHECK_EQ(c.StartY(), y) << c.StartY() << " " << y;
      // Because the path was closed, our cursor is already
      // on startx, starty.
      for (int i = 0; i <= bestidx; i++) {
        r.paths.push_back(c.paths[i]);
        x = c.paths[i].x;
        y = c.paths[i].y;
      }
      out.push_back(std::move(r));
    }
  }
  return out;
}

TTF::Contour TTF::ReverseContour(const Contour &c) {
  // Intermediate representation as [start, control, end]
  using Point = pair<float, float>;
  using Segment = tuple<Point, optional<Point>, Point>;
  vector<Segment> segments;

  Point prev = make_pair(c.StartX(), c.StartY());
  for (const Path &p : c.paths) {
    Point next = make_pair(p.x, p.y);
    switch (p.type) {
    case PathType::LINE:
      segments.emplace_back(prev, nullopt, next);
      break;
    case PathType::BEZIER:
      segments.push_back(Segment(prev, {make_pair(p.cx, p.cy)}, next));
      break;
    default:
      CHECK(false) << "Bad path type";
    }
    prev = next;
  }

  Contour out;
  out.paths.reserve(c.paths.size());
  for (int i = segments.size() - 1; i >= 0; i--) {
    const auto &[start, ctrl, end_] = segments[i];
    if (ctrl.has_value()) {
      // Bezier
      out.paths.emplace_back(start.first, start.second,
                             ctrl.value().first, ctrl.value().second);
    } else {
      // Line
      out.paths.emplace_back(start.first, start.second);
    }
  }
  return out;
}

bool TTF::IsClockwise(const Contour &c) {
  float x = c.StartX();
  float y = c.StartY();
  float sum = 0.0f;
  for (const Path &p : c.paths) {
    float dx = p.x - x;
    float sy = p.y + y;
    sum += dx * sy;
    x = p.x;
    y = p.y;
  }
  return sum < 0.0f;
}

ImageA TTF::GetChar(int codepoint, int size) {
  int width, height;
  uint8_t *bitmap =
    stbtt_GetCodepointBitmap(
        &font, 0, stbtt_ScaleForPixelHeight(&font, size), codepoint,
        &width, &height, 0, 0);
  CHECK(bitmap != nullptr) << "Codepoint " << codepoint << " size " << size;

  const int bytes = width * height;
  std::vector<uint8_t> ret;
  ret.resize(bytes);
  memcpy(ret.data(), bitmap, bytes);
  stbtt_FreeBitmap(bitmap, nullptr);
  return ImageA(std::move(ret), width, height);
}


std::optional<ImageA> TTF::GetSDF(int codepoint,
                                  int sdf_size,
                                  int pad_top, int pad_bot, int pad_left,
                                  uint8_t onedge_value,
                                  float falloff_per_pixel) const {
  CHECK(sdf_size > 0);
  CHECK(pad_top >= 0 && pad_top < sdf_size);
  CHECK(pad_left >= 0 && pad_left < sdf_size);
  CHECK(pad_bot >= 0 && pad_bot < sdf_size);
  const int ht = sdf_size - (pad_top + pad_bot);
  CHECK(ht > 0);

  // Location of origin in output.
  const int sdf_ox = pad_left;
  const int sdf_oy = pad_top + ht;

  // PERF: Internally, we pad with sdf_size on every side, which guarantees
  // we'll have enough pixels as long as the origin is inside the
  // bitmap. But this is probably overkill!
  const int stb_pad = sdf_size;
  const float stb_scale = stbtt_ScaleForPixelHeight(&font, ht);

  int width, height, xoff, yoff;
  uint8_t *bit = stbtt_GetCodepointSDF(&font,
                                       stb_scale,
                                       codepoint,
                                       // padding
                                       stb_pad,
                                       // onedge value
                                       onedge_value,
                                       // falloff per pixel
                                       falloff_per_pixel,
                                       &width, &height,
                                       &xoff, &yoff);
  // This just fails for some fonts?
  if (bit == nullptr)
    return {};

  // printf("stb stf: %dx%d, off %d %d\n", width, height, xoff, yoff);

  ImageA out{sdf_size, sdf_size};

  // Now fill the whole sdf image.
  // sx,sy are pixel coordinates in the output sdf.
  // bx,by are pixel coordinates in the stb sdf bitmap.
  // Note xoff/yoff are typically negative (whereas sdf_o(x,y) are positive;
  // the coordinates of the origin within the bitmap.
  for (int sy = 0; sy < sdf_size; sy++) {
    const int by = (sy - sdf_oy) + (-yoff);
    // output sdf isn't covered
    if (by < 0 || by >= height) {
      stbtt_FreeSDF(bit, nullptr);
      return {};
    }

    for (int sx = 0; sx < sdf_size; sx++) {
      const int bx = (sx - sdf_ox) + (-xoff);

      // output sdf isn't covered
      if (bx < 0 || bx >= width) {
        stbtt_FreeSDF(bit, nullptr);
        return {};
      }
      out.SetPixel(sx, sy, bit[by * width + bx]);
    }
  }

  // Check that there are not interior points that fall outside
  // the output sdf.
  // Here sx,sy,bx,by have the same meaning as above, but we loop
  // over bitmap pixels and convert to sdf pixels.
  for (int by = 0; by < height; by++) {
    const int sy = by + yoff + sdf_oy;
    for (int bx = 0; bx < width; bx++) {
      const int sx = bx + xoff + sdf_ox;

      uint8_t v = bit[by * width + bx];
      if (v >= onedge_value) {
        if (sy < 0 || sy >= sdf_size ||
            sx < 0 || sx >= sdf_size) {
          // Some interior pixel was outside the output SDF.
          // Would need more padding (or something else is weird).
          stbtt_FreeSDF(bit, nullptr);
          return {};
        }
      }
    }
  }

  stbtt_FreeSDF(bit, nullptr);
  return {out};
}


// Can be empty (e.g. for 'space' character)
std::vector<TTF::NativeContour> TTF::GetNativeContours(int codepoint) const {
  stbtt_vertex *vertices = nullptr;
  const int n = stbtt_GetCodepointShape(&font, codepoint, &vertices);
  if (n == 0) return {};
  CHECK(vertices != nullptr);

  CHECK(vertices[0].type == STBTT_vmove) <<
    "All shapes should start with a moveto?";
  std::optional<NativeContour> cur;
  std::vector<NativeContour> out;
  for (int i = 0; i < n; i++) {
    const stbtt_vertex &v = vertices[i];
    switch (v.type) {
    case STBTT_vmove:
      if (cur.has_value()) {
        out.emplace_back(cur.value());
      }
      cur.emplace();
      break;
    case STBTT_vline:
      CHECK(cur.has_value());
      cur.value().paths.emplace_back(v.x, v.y);
      break;
    case STBTT_vcurve:
      // printf("vcurve\n");
      CHECK(cur.has_value());
      cur.value().paths.emplace_back(v.x, v.y, v.cx, v.cy);
      break;
    case STBTT_vcubic:
      CHECK(false) << "Expected only quadratic splines in TTF :(";
      break;
    }
  }
  CHECK(cur.has_value());
  out.emplace_back(cur.value());

  stbtt_FreeShape(&font, vertices);
  return out;
}

// Map from codepoint to a nicer name for the character. All ASCII
// is covered. FontForge seems to accept e.g. U+2658 too, so outside
// ASCII we can just use that.
static const std::unordered_map<int, string> &CharNames() {
  static const auto *kCharNames =
    new std::unordered_map<int, string>{
    {32, "space"},
    {33, "exclam"},
    {34, "quotedbl"},
    {35, "numbersign"},
    {36, "dollar"},
    {37, "percent"},
    {38, "ampersand"},
    {39, "quotesingle"},
    {40, "parenleft"},
    {41, "parenright"},
    {42, "asterisk"},
    {43, "plus"},
    {44, "comma"},
    {45, "hyphen"},
    {46, "period"},
    {47, "slash"},
    {48, "zero"},
    {49, "one"},
    {50, "two"},
    {51, "three"},
    {52, "four"},
    {53, "five"},
    {54, "six"},
    {55, "seven"},
    {56, "eight"},
    {57, "nine"},
    {58, "colon"},
    {59, "semicolon"},
    {60, "less"},
    {61, "equal"},
    {62, "greater"},
    {63, "question"},
    {64, "at"},
    {65, "A"},
    {66, "B"},
    {67, "C"},
    {68, "D"},
    {69, "E"},
    {70, "F"},
    {71, "G"},
    {72, "H"},
    {73, "I"},
    {74, "J"},
    {75, "K"},
    {76, "L"},
    {77, "M"},
    {78, "N"},
    {79, "O"},
    {80, "P"},
    {81, "Q"},
    {82, "R"},
    {83, "S"},
    {84, "T"},
    {85, "U"},
    {86, "V"},
    {87, "W"},
    {88, "X"},
    {89, "Y"},
    {90, "Z"},
    {91, "bracketleft"},
    {92, "backslash"},
    {93, "bracketright"},
    {94, "asciicircum"},
    {95, "underscore"},
    {96, "grave"},
    {97, "a"},
    {98, "b"},
    {99, "c"},
    {100, "d"},
    {101, "e"},
    {102, "f"},
    {103, "g"},
    {104, "h"},
    {105, "i"},
    {106, "j"},
    {107, "k"},
    {108, "l"},
    {109, "m"},
    {110, "n"},
    {111, "o"},
    {112, "p"},
    {113, "q"},
    {114, "r"},
    {115, "s"},
    {116, "t"},
    {117, "u"},
    {118, "v"},
    {119, "w"},
    {120, "x"},
    {121, "y"},
    {122, "z"},
    {123, "braceleft"},
    {124, "bar"},
    {125, "braceright"},
    {126, "asciitilde"},
  };
  return *kCharNames;
}


// This is a quick-and-dirty export. Basically we have to generate a
// minimal header (should expose some of this as parameters) and
// convert the float representation here back into an integer one that
// FontForge will be happy with. The other issue is that the
// coordinate system is inverted in the y axis, so we should reverse
// the order of all contours in order to preserve their winding order.
//
// SFD format here: https://fontforge.org/docs/techref/sfdformat.html
//
// TODO: Add support for kerning. This looks reasonably easy:
//   In the header, add a kerning table with subtable:
// Lookup: 258 0 0 "'kern' kerning table" { "kerning subtable" [614,61,6] } ['kern' ('DFLT' <'dflt' > 'latn' <'dflt' > ) ]
// [614,61,6] appear to be the default separation (614) and min kern (61),
// and some flags for that menu (6), which I think are just FontForge
// settings. So for generating fonts we can just put whatever here. These
// specific flags disable autokerning, in case that happens on export?
// Lookup: 258 0 0 is lookup type and flags, which should probably
// just be verbatim.
//
// Then the kerning data is stored in each glyph. For example we have
//
//     StartChar: T
//     Encoding: 84 84 52
//     Width: 2560
//     Flags: W
//     LayerCount: 2
//     Fore
//     SplineSet
//     0 2560 m 0,0,-1
//       ...
//      0 2560 l 0,0,-1
//     EndSplineSet
//     Kerns2: 79 -782 "kerning subtable"
//     EndChar
//
// 79 is the character o, and -782 is the negative offset.
string TTF::Font::ToSFD(const string &name) const {
  // TTF uses integral coordinates. This is the size of the box
  // we discretize to. Larger numbers here mean less lost precision,
  // but since we are using 16 bit coordinates and it is normal for
  // values to exceed the box, we don't want a number that's too high.
  //
  // (XXX When we generate fonts from bitmap data, it's probably better
  // to use some multiple of the original grid exactly. Make this
  // configurable? See for example that vertices on the baseline have
  // y=-1 in DFX snooty 10px)

  int box = 4096;
  if (bitmap_grid_height > 0) {
    box = bitmap_grid_height;
    while (box < 400) box *= 10;
  }

  // At least for SFD, ascent and descent seem to both be defined as
  // both positive. They are determined from the baseline.
  const int ascent = baseline * box;
  int descent = box - ascent;
  CHECK(baseline > 0.0f && baseline < 1.0f && ascent && descent) <<
    "This code is not set up to handle baselines outside the box, "
    "or zero ascent/descent.";

  // TODO: compute underline position/width from box
  int native_linegap = linegap * box;
  if (native_linegap < 0) {
    // Doesn't seem to work in FontForge to have negative linegap,
    // so instead subtract this from the descent.
    descent += native_linegap;
    native_linegap = 0;
  }

  CHECK(descent >= 0) << "negative linegap removed entire descent?";

  string name_no_space;
  for (int i = 0; i < (int)name.size(); i++) {
    char c = name[i];
    if (c != ' ') name_no_space += c;
  }

  // In FontForge 20220308, AntiAlias: 0 crashes the program on load.
  // This is presumably a bug.
  // const char antialias_flag = antialias ? '1' : '0';
  const char antialias_flag = '1';

  // FYI the values in the Layer: command are what tell it that
  // we are using quadratic beziers.
  const int numchars = chars.size();
  string out = StringPrintf(
R"!(SplineFontDB: 3.2
FontName: %s
FullName: %s
FamilyName: %s
Weight: Regular
Copyright: %s
UComments: "Generated by cc-lib ttf.cc"
Version: 001.000
ItalicAngle: 0
UnderlinePosition: -100
UnderlineWidth: 50
Ascent: %d
Descent: %d
InvalidEm: 0
LayerCount: 2
Layer: 0 1 "Back" 1
Layer: 1 1 "Fore" 0
OS2Version: 0
OS2_WeightWidthSlopeOnly: 0
OS2_UseTypoMetrics: 1
PfmFamily: 0
TTFWeight: 0
TTFWidth: 0
LineGap: %d
VLineGap: %d
OS2TypoAscent: 0
OS2TypoAOffset: 1
OS2TypoDescent: 0
OS2TypoDOffset: 1
OS2TypoLinegap: %d
OS2WinAscent: 0
OS2WinAOffset: 1
OS2WinDescent: 0
OS2WinDOffset: 1
HheadAscent: 0
HheadAOffset: 1
HheadDescent: 0
HheadDOffset: 1
OS2Vendor: '%c%c%c%c'
DEI: 91125
Encoding: UnicodeFull
UnicodeInterp: none
NameList: AGL For New Fonts
DisplaySize: -48
AntiAlias: %c
FitToEm: 0
WinInfo: 64 8 5
BeginChars: 1114112 %d
)!", name_no_space.c_str(), name.c_str(), name.c_str(),
     copyright.c_str(),
     ascent, descent,
     native_linegap, native_linegap, native_linegap,
     vendor[0], vendor[1], vendor[2], vendor[3],
     antialias_flag,
     numchars);

  auto MapX = [this, box](float x) -> int {
      return roundf(x * box * extra_scale);
    };
  auto MapY = [this, box, ascent](float y) -> int {
      // 0 should map to ascent
      // baseline should map to zero

      y *= box;
      y -= ascent;
      y = -y;

      return roundf(y * extra_scale);
    };

  auto CharToSFD = [&](int codepoint, const Char &ch, int index) {
      string char_name;
      {
        const auto &names = CharNames();
        auto it = names.find(codepoint);
        if (it == names.end()) {
          if (codepoint <= 0xFFFF) {
            char_name = StringPrintf("U+%04x", codepoint);
          } else {
            // This is what FontForge output for U+1F680 (rocket emoji),
            // at least.
            char_name = StringPrintf("u%X", codepoint);
          }
        } else {
          // In map; use the preferred name.
          char_name = it->second;
        }
      }
      int width = MapX(ch.width);
      string ret = StringPrintf(
          "\n"
          "StartChar: %s\n"
          // decimal unicode codepoint twice, then index in file
          "Encoding: %d %d %d\n"
          // Explicit width
          "Flags: W\n"
          "Width: %d\n"
          "Layercount: 2\n"
          "Fore\n",
          char_name.c_str(),
          codepoint, codepoint, index,
          width);

      if (!ch.contours.empty()) {
        ret += "SplineSet\n";
        for (const Contour &ucontour : ch.contours) {
          // Flipping y axis, so orientation of clockwise changes.
          const Contour contour = ucontour; // ReverseContour(ucontour);
          StringAppendF(&ret, "%d %d m 0\n",
                        MapX(contour.StartX()),
                        MapY(contour.StartY()));
          for (const Path &path : contour.paths) {
            int x = MapX(path.x);
            int y = MapY(path.y);
            switch (path.type) {
            case PathType::LINE:
              StringAppendF(&ret, " %d %d l 0\n", x, y);
              break;
            case PathType::BEZIER: {
              int cx = MapX(path.cx);
              int cy = MapY(path.cy);
              // Seems that FontForge wants two control points (this is how it saves
              // quad beziers itself), so duplicate.
              StringAppendF(&ret, " %d %d %d %d %d %d c 0\n", cx, cy, cx, cy, x, y);
              break;
            }
            }
          }
        }
        ret += "EndSplineSet\n";
      }

      ret += "EndChar\n";

      return ret;
    };

  int index = 0;
  for (const auto &[cp, ch] : chars) {
    out += CharToSFD(cp, ch, index);
    index++;
  }

  out +=
    "EndChars\n"
    "EndSplineFont\n";

  return out;
}

