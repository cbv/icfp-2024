
#include "fonts/island-finder.h"

#include <vector>
#include <utility>
#include <initializer_list>
#include <cstdint>
#include <map>
#include <set>

#include "base/logging.h"
#include "image.h"

using uint8 = uint8_t;
using uint16 = uint16_t;

using namespace std;

// If true, do some internal checking to catch bugs
static constexpr bool VALIDATE = false;

// (Implementation notes here were from this code's use in tracing SDFs
// in ../../lowercase/font-problem. Perhaps should update these to make
// them more generic...)

// Imagine a series of nested islands, where the border is a transition
// (along the grid edges, 4-connected) between less than onedge_value
// and greater-or-equal to onedge_value. The nesting alternates "sea"
// and "land", and an island can contain zero or more child islands.
//
// Goal is to fill in every pixel of the SDF with an equivalence class
// id and parent pointer. The ids distinguish connected components in
// the case that there are multiple at the same depth.
//
// The outside of the SDF is defined as sea--island zero--which may
// connect with pixels inside the image. We initialize by creating
// this boundary around the SDF and doing the flood-fill inside it;
// as we continue we will do lower depths first, which gives the
// algorithm its direction (and termination condition).
//
// In the flood fill, we pick a pixel with the lowest depth and expand.
// Pixels in the queue already have an equivalence class (but maybe not
// a final one) and parent pointer assigned. For their connected
// neighbors:
//  - If there's a border:
//     - If already processed, should be nothing to do. (It should
//       end up being our parent or child, but maybe not resolved yet?)
//     - Otherwise, because we do this in depth order, this is
//       a child of ours. Set it at depth+1, parent pointing to us,
//       and a new equivalence class.
//  - If there's no border:
//     - If already processed, join the equivalence classes.
//     - If not, join the equivalence classes, copy our parent pointer
//       and depth to it, and enqueue.
//
// And actually it seems that the parent pointer is a property of the
// equivalence class, although it's not known until a pixel is
// processed.
namespace {
struct Todo {
  void Add(int depth, int idx) {
    m[depth].push_back(idx);
  }

  bool HasNext() const {
    return !m.empty();
  }

  std::pair<int, int> Next() {
    CHECK(!m.empty());
    auto it = m.begin();
    const int depth = it->first;
    const int idx = it->second.back();
    it->second.pop_back();
    if (it->second.empty()) {
      m.erase(it);
    }
    return {depth, idx};
  }

  // Map of depth to to-do list for that depth.
  // Representation invariant: no vectors are empty.
  std::map<int, vector<int>> m;
};
}

IslandFinder::IslandFinder(const ImageA &bitmap) :
  img(Preprocess(bitmap)),
  radix(img.Width() * img.Height()) {
  for (int i = 0; i < radix; i++) {
    eqclass.push_back(-1);
    classinfo.push_back(nullopt);
  }
}

int IslandFinder::Height() const { return img.Height(); }
int IslandFinder::Width() const { return img.Width(); }

std::pair<int, int> IslandFinder::GetXY(int index) const {
  return make_pair(index % img.Width(), index / img.Width());
}

int IslandFinder::Index(int x, int y) const {
  return y * img.Width() + x;
}

bool IslandFinder::IsLand(int x, int y) const {
  if (x < 0 || y < 0 || x >= img.Width() || y >= img.Height())
    return false;
  else return img.GetPixel(x, y) > 0;
}

bool IslandFinder::Visited(int idx) const {
  return classinfo[idx].has_value();
}

void IslandFinder::SetInfo(int idx, int depth, int parent) {
  CHECK(!classinfo[idx].has_value()) << idx << " depth "
                                     << depth << " par " << parent;
  classinfo[idx].emplace(depth, parent);
}

IslandFinder::Info IslandFinder::GetInfo(int idx) const {
  CHECK(classinfo[idx].has_value());
    return classinfo[idx].value();
}

int IslandFinder::GetClass(int idx) {
  CHECK(idx >= 0 && idx < (int)eqclass.size());
  if (eqclass[idx] == -1) return idx;
  else return eqclass[idx] = GetClass(eqclass[idx]);
}

void IslandFinder::Union(int aidx, int bidx) {
  CHECK(aidx >= 0 && aidx < (int)eqclass.size());
  CHECK(bidx >= 0 && bidx < (int)eqclass.size());

  CHECK(Visited(aidx));
  CHECK(Visited(bidx));

  int a_class = GetClass(aidx);
  int b_class = GetClass(bidx);
  if (a_class != b_class) {
    CHECK(Visited(a_class));
    CHECK(Visited(b_class));
    // Check that the classes are compatible.
    CHECK(classinfo[a_class].value().depth ==
          classinfo[b_class].value().depth);
    // It's probably the cases that the parent
    // classes are actually equal too?
    eqclass[a_class] = b_class;
  }
}

void IslandFinder::Fill() {
  const int TOP_LEFT = Index(0, 0);
  // Parent of outermost region is itself.
  SetInfo(TOP_LEFT, 0, TOP_LEFT);

  // Start by marking the border as visited, and part of the same
  // island. This prevents us from going off the map, and ensures
  // that the outer region is all connected.
  // Left and right edges.
  auto MarkBoundary = [this, TOP_LEFT](int x, int y) {
      int idx = Index(x, y);
      SetInfo(idx, 0, TOP_LEFT);
      Union(TOP_LEFT, idx);
    };
  
  for (int y = 0; y < Height(); y++) {
    if (y != 0)
      MarkBoundary(0, y);
    MarkBoundary(Width() - 1, y);
  }
  // Top and bottom edges.
  for (int x = 1; x < Width() - 1; x++) {
    MarkBoundary(x, 0);
    MarkBoundary(x, Height() - 1);
  }

  // To kick off the process, we start with the pixel at (1,1). It
  // must be sea because we padded the whole thing with two pixels
  // of sea.
  CHECK(!IsLand(1, 1));

  const int start_idx = Index(1, 1);
  SetInfo(start_idx, 0, TOP_LEFT);
  Union(TOP_LEFT, start_idx);

  Todo todo;
  todo.Add(0, Index(1, 1));

  // Now, the steady state flood-fill.
  while (todo.HasNext()) {
    const auto [src_depth, src_idx] = todo.Next();
    CHECK(Visited(src_idx));

    const int src_parent = GetInfo(src_idx).parent_class;

    const auto [src_x, src_y] = GetXY(src_idx);
    for (const auto &[dx, dy] : initializer_list<pair<int, int>>{
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}}) {
      const int x = src_x + dx;
      const int y = src_y + dy;
      const int idx = Index(x, y);

      const bool has_border =
        IsLand(src_x, src_y) != IsLand(x, y);

      if (has_border) {
        if (Visited(idx)) {
          Info info = GetInfo(idx);
          const int depth = info.depth;
          // We could be reaching this pixel the second time (like if
          // it's a corner, or on a one-wide strip), or we could be
          // looking out of the outer edge of our own region.
          if (depth == src_depth + 1) {
            // If this is an island within this one, enforce that
            // it has a single parent, by potentially joining the
            // current area with its parent. This is important for
            // cases like
            //
            //     ##
            //   ##  ##
            //   ##  ##
            //     ##
            //
            // where the interior is not 4-connected to the exterior,
            // but neither are the four blocks that cause it to be
            // disconnected. The best way to treat this is as a single
            // island with a single hole in it; this causes the four
            // blocks to be joined as they discover the hole.
            Union(src_idx, info.parent_class);

          } else if (depth == src_depth - 1) {
              // Nothing to do (or some sanity checking at least?)

          } else {
            CHECK(false) << "Upon reaching a pixel for the second "
              "time, it should be depth - 1 or depth + 1!";
          }
        } else {
          // Otherwise, we found part of an island within this one.
          SetInfo(idx, src_depth + 1, src_idx);
          todo.Add(src_depth + 1, idx);
        }
      } else {
        // If we haven't visited it yet, initialize at the same
        // depth and put it in the queue.
        if (!Visited(idx)) {
          SetInfo(idx, src_depth, src_parent);
          todo.Add(src_depth, idx);
        }

        // And either way, since we are connected neighbors, join
        // the equivalence classes.
        Union(src_idx, idx);
      }
    }
  }
}

std::tuple<ImageA, ImageA, std::map<uint8, uint8>>
IslandFinder::GetMaps() {
  const int w = img.Width() - 4;
  const int h = img.Height() - 4;
  ImageA depth(w, h);
  ImageA eq(w, h);
  std::map<uint8, uint8> parent;

  const int zero_id = GetClass(Index(0, 0));
  std::unordered_map<int, uint8> toeq = {{zero_id, 0}};
  uint8 next_id = 1;
  auto GetEq = [&toeq, &next_id](int eqc) {
      auto it = toeq.find(eqc);
      if (it == toeq.end()) {
        CHECK(next_id < 255);
        uint8 ret = next_id++;
        toeq[eqc] = ret;
        return ret;
      } else {
        return it->second;
      }
    };

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int yy = y + 2;
      int xx = x + 2;
      int idx = Index(xx, yy);
      CHECK(Visited(idx)) << "bug? or call Fill";
      Info info = GetInfo(idx);
      CHECK(info.depth >= 0 && info.depth < 256) << info.depth;
      depth.SetPixel(x, y, info.depth);
      const uint8 eq8 = GetEq(GetClass(idx));
      eq.SetPixel(x, y, eq8);
      // Don't set parent for class 0.
      if (eq8 != 0) {
        CHECK(info.parent_class >= 0);
        const uint8 pq8 = GetEq(GetClass(info.parent_class));
        auto it = parent.find(eq8);
        if (it != parent.end()) {
          CHECK(it->second == pq8) << "at " << x << "," << y <<
            ", Inconsistent parents for equivalence class " <<
            GetClass(idx) << " --> " << (int)eq8 <<
            ": " << GetClass(info.parent_class) << " --> " << (int)pq8 <<
            " but already had (something) --> " << (int)it->second << "!";
        } else {
          parent[eq8] = pq8;
        }
      }
    }
  }
  return make_tuple(depth, eq, parent);
}

ImageRGBA IslandFinder::DebugBitmap() {
  ImageRGBA out(img.Width(), img.Height());
  out.Clear32(0x000000FF);
  std::vector<uint16_t> COLORS_LEFT = {
    0xFF00,
    0x00FF,
    0x4400,
    0x0044,
    0x9900,
    0x0099,
    0xAA00,
    0x00AA,
    0x44FF,
    0xFF44,
    0x4444,
    0x9999,
    0xAAAA,
  };
  std::unordered_map<int, uint16> colors;
  for (int y = 0; y < img.Height(); y++) {
    for (int x = 0; x < img.Width(); x++) {
      int idx = Index(x, y);
      if (Visited(idx)) {
        Info info = GetInfo(idx);
        uint8 r = info.depth * 0x20;
        // green and blue from eq class.
        int eq = GetClass(idx);
        uint16 gb = 0;
        auto c = colors.find(eq);
        if (c == colors.end()) {
          CHECK(!COLORS_LEFT.empty());
          gb = COLORS_LEFT.back();
          colors[eq] = gb;
          COLORS_LEFT.pop_back();
        } else {
          gb = c->second;
        }
        uint8 g = (gb >> 8) & 0xFF;
        uint8 b = gb & 0xFF;
        out.SetPixel(x, y, r, g, b, 0xFF);
      } else {
        out.SetPixel32(x, y, 0xFF0000FF);
      }
    }
  }
  return out;
}

ImageA IslandFinder::Preprocess(const ImageA &bitmap) {
  ImageA ret(bitmap.Width() + 4, bitmap.Height() + 4);
  ret.Clear(0);
  ret.BlendImage(2, 2, bitmap);
  return ret;
}


IslandFinder::Maps IslandFinder::Find(const ImageA &bitmap,
                                      ImageRGBA *islands_debug) {
  IslandFinder finder(bitmap);
  finder.Fill();
  if (islands_debug != nullptr) *islands_debug = finder.DebugBitmap();

  // XXX just do this directly
  const auto [dm, em, p] = finder.GetMaps();
  std::tuple<ImageA, ImageA, std::map<uint8, uint8>> m = finder.GetMaps();
  Maps maps;
  maps.depth = std::move(dm);
  maps.eqclass = std::move(em);
  maps.parentmap = std::move(p);


  if (VALIDATE) {
    std::set<uint8> all;
    for (int y = 0; y < maps.eqclass.Height(); y++) {
      for (int x = 0; x < maps.eqclass.Width(); x++) {
        uint8 eqc = maps.eqclass.GetPixel(x, y);
        all.insert(eqc);
      }
    }
    for (const auto [child, parent] : maps.parentmap) {
      all.insert(child);
      all.insert(parent);
    }

    for (const uint8 eqc : all) {
      if (eqc != 0) {
        CHECK(maps.parentmap.find(eqc) !=
              maps.parentmap.end()) <<
          "Bug: Didn't find equivalence class " << (int)eqc <<
          " in the parent map!";
      }
    }

    printf("Validate ok. Classes:\n");
    for (const uint8 eqc : all) {
      printf("%d ", eqc);
    }
    printf("\n");
  }
  
  // Get the depth of each equivalence class that occurs, and the
  // maximum depth.
  maps.max_depth = 0;
  for (int y = 0; y < maps.depth.Height(); y++) {
    for (int x = 0; x < maps.depth.Width(); x++) {
      uint8 d = maps.depth.GetPixel(x, y);
      uint8 eqc = maps.eqclass.GetPixel(x, y);
      auto it = maps.eqclass_depth.find(eqc);
      if (it == maps.eqclass_depth.end()) {
        maps.eqclass_depth[eqc] = d;
      } else {
        CHECK(it->second == d) << "Inconsistent depth for eqclass "
                               << (int)eqc << " at " << x << "," << y;
      }
      maps.max_depth = std::max((int)d, maps.max_depth);
    }
  }

  return maps;
}

uint8 IslandFinder::Maps::GetAncestorAtDepth(uint8 d, uint8 eqc) const {
  for (;;) {
    if (d == 0) return 0;
    auto dit = eqclass_depth.find(eqc);
    CHECK(dit != eqclass_depth.end()) << (int)eqc << " has no depth?";
    if (dit->second == d) return eqc;

    auto pit = parentmap.find(eqc);
    CHECK(pit != parentmap.end()) << (int)eqc << " has no parent?";
    eqc = pit->second;
  }
}

// PERF Seems an explicit tree structure would have been better, but these
// maps tend to be very small, so no big deal to keep doing lookups...
bool IslandFinder::Maps::HasAncestor(uint8 eqc, uint8 parent) const {
  for (;;) {
    // First test this, allowing 0 as a parent.
    if (eqc == parent) return true;
    // ... but if we are not looking for 0, this means we reached
    // the root.
    if (eqc == 0) return false;
    
    auto pit = parentmap.find(eqc);
    CHECK(pit != parentmap.end()) << (int)eqc << " has no parent?";
    eqc = pit->second;
  }
}
