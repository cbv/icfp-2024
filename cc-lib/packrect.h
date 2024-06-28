
#ifndef _CC_LIB_PACKRECT_H
#define _CC_LIB_PACKRECT_H

#include <utility>
#include <vector>

// Packs rectangles into a rectangle without overlap. Commonly used to
// put sprites or textures for a game into a single image. Has both
// "fast" (development) and "optimize" (shipping) settings.
//
// Note that because we don't allow any overlap (and thus don't care
// about the image contents), we lose the opportunity to do certain
// kinds of optimizations that can be helpful (such as sharing
// transparent regions between two images).

// Note that currently the "optimization" mode does something but is
// not interesting or efficient (since there are only two algorithms
// internally), and it might significantly exceed the configured
// budgets. TODO: Improve!

struct PackRect {

  struct Config {
    // Constraints on the output size. If zero, unconstrained.
    int max_width = 0;
    int max_height = 0;

    // If >0, stop after this many attempts. Setting this to 1 causes
    // it to finish with a reasonable result quickly.
    int budget_passes = 1;

    // If >0, stop after we have exceeded this number of (walltime)
    // seconds. Good for "optimized"
    int budget_seconds = 0;
  };

  // Returns false if packing failed, which will only be the case (for
  // valid inputs) if max_width or max_height is constrained but we
  // cannot find a rectangle packing that fits.
  static bool Pack(
      Config config,
      // Rectangles to pack, given a pair width,height.
      const std::vector<std::pair<int, int>> &rects,
      // Size of the output rectangle.
      int *width, int *height,
      // The output positions (x,y), parallel to the input rects.
      std::vector<std::pair<int, int>> *positions);

};


#endif
