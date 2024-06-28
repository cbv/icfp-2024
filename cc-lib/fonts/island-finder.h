
#ifndef _CC_LIB_FONTS_ISLAND_FINDER_H
#define _CC_LIB_FONTS_ISLAND_FINDER_H

#include <utility>
#include <map>
#include <vector>
#include <cstdint>
#include <tuple>
#include <unordered_map>

#include "image.h"


// Island Finder takes a 1-bit bitmap image and breaks it into nested
// equivalence classes ("islands") of connected components. The typical
// application is processing font glyphs.

struct IslandFinder {

  // The result of computing islands, represented with 8-bit bitmaps
  // (imposing some reasonable limits on max depth, etc.).
  struct Maps {
    // These two bitmaps are the same size as the original image.
    // Value is the depth. Depth zero is "outside."
    ImageA depth;
    // Equivalence class for each pixel. Value is an arbitrary unique
    // id, though depth zero will always be eqclass 0.
    ImageA eqclass;
    //  - A map from each equivalence class in the second image
    //    to its parent equivalence class. The exception is 0,
    //    which has no parent and does not appear in the map.
    std::map<uint8_t, uint8_t> parentmap;

    // Derived from above.

    // The depth of each equivalence class.
    std::unordered_map<uint8_t, uint8_t> eqclass_depth;
    int max_depth = 0;

    // Assuming the depth of eqc is at least d, get the ancestor
    // that is at depth exactly d.
    uint8_t GetAncestorAtDepth(uint8_t d, uint8_t eqc) const;

    // XXX docs
    bool HasAncestor(uint8_t eqc, uint8_t parent) const;
  };

  // Compute the islands.
  // The input bitmap is 8-bit, but we only treat it as 1-bit (0 =
  // sea, >0 = land). Behaves as though the image is surrounded
  // by sea.
  //
  // If islands_debug is non-null, it's populated with an image that
  // visualizes the internal processes.
  static Maps Find(const ImageA &bitmap,
                   ImageRGBA *islands_debug = nullptr);


  // TODO: No need to expose these details any more...
 private:
  // Information about a pixel, which should be eventually
  // shared with the full equivalence class.
  struct Info {
    Info(int d, int pc) : depth(d), parent_class(pc) {}
    int depth = -1;
    // Note: need to resolve this with GetClass, as the
    // parent could be unioned with something later (is
    // it actually possible? well in any case, it is right
    // to look up the class).

    int parent_class = -1;
  };

  // Pre-processed bitmap with pixels >0 for land, =0 for sea. Padded
  // such that it has two pixels of sea on every side.
  const ImageA img;
  const int radix;

  // Same size as image. Gives the pixel's current equivalence
  // class; each one starts in its own (value -1) to begin.
  // Union-find-like algorithm.
  std::vector<int> eqclass;

  // For each pixel, nullopt if we have not yet visited it.
  // If we have visited, its depth (will never change) and
  // parent pixel (must canonicalize via GetClass).
  std::vector<std::optional<Info>> classinfo;

  void SetInfo(int idx, int depth, int parent);
  void Union(int aidx, int bidx);

  // Many of these could be public, but they are
  std::pair<int, int> GetXY(int index) const;
  int Index(int x, int y) const;

  bool IsLand(int x, int y) const;


  // private?
  bool Visited(int idx) const;

  Info GetInfo(int idx) const;

  int Height() const;
  int Width() const;

  // Return the equivalence class that this index currently
  // belongs in. Not const because it performs path compression.
  int GetClass(int idx);

  static ImageA Preprocess(const ImageA &bitmap);

  // The input bitmap is 8-bit, but we only treat it as 1-bit (0 =
  // sea, >0 = land). The image is automatically padded with two
  // pixels of sea on all sides, so Width() below will be
  // bitmap.Width() + 4.
  //
  // Typical usage is to immediately call Fill and then work
  // with the results of GetMaps.
  explicit IslandFinder(const ImageA &bitmap);

  // Usually you want to immediately run Fill after constructing.
  void Fill();

  // Get the output of this process as three components:
  //  - Two bitmaps the size of the original bitmap:
  //      (removing the padding used for internal purposes).
  //    - Depth map. Value is the depth.
  //    - Equivalence classes. Value is arbitrary unique id,
  //      though depth zero will be equivalence class 0.
  // Since the output is 8-bit, there can only be up to 255
  // in each case.
  std::tuple<ImageA, ImageA, std::map<uint8_t, uint8_t>> GetMaps();

  // For debugging, generate a visualization of the Fill process.
  // For clients, call after Fill(), but it is also possible to call
  // during Fill (by modifying the code herein).
  ImageRGBA DebugBitmap();
};


#endif
