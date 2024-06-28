
#ifndef _CC_LIB_BOUNDS_H
#define _CC_LIB_BOUNDS_H

#include <cstdint>
#include <utility>
#include <limits>
#include <utility>

// TODO: We now have a notion of "empty in x" and "empty in y",
// which should be preserved by Union and margins (in that dimension).
// Currently these treat the bounding box as empty if either dimension
// is empty.

// Imperative 2D bounding box, after sml-lib.
struct Bounds {
  // With no points.
  Bounds();

  // Expand the bounding box to contain the point.
  // Ignores NaN.
  void Bound(double x, double y);
  void Bound(std::pair<double, double> p);
  void BoundX(double x);
  void BoundY(double y);

  // The margins are included.
  bool Contains(double x, double y) const;

  // Returns true if we don't have points on both the x and y axes
  // (because Bound has not been called, or not both of BoundX and BoundY).
  // When the bounding box is empty, several functions below should not
  // be called.
  bool Empty() const;

  double MinX() const;
  double MinY() const;
  double MaxX() const;
  double MaxY() const;

  // The offset of the input point within the bounding box (thinking
  // of MinX/MinY as 0,0).
  double OffsetX(double x) const;
  double OffsetY(double y) const;

  double Width() const;
  double Height() const;

  // Modifies 'this'. 'this' and 'other' may be empty.
  void Union(const Bounds &other);

  // Adds a fixed-sized margin around the entire bounds, in absolute units.
  // If empty, does nothing. d must be non-negative.
  void AddMargin(double d);
  // Add margin to the sides of the bounding box.
  // If empty, does nothing. Arguments must be non-negative.
  void AddMargins(double up, double right, double down, double left);

  // Add a margin on both sides of each dimension that's a fraction
  // (e.g. 0.01 for 1% on each side) of its current width. If empty,
  // does nothing. f must be non-negative.
  void AddMarginFrac(double f);
  void AddMarginsFrac(double fup, double fright,
                      double fdown, double fleft);

  // A common thing to do is collect some points into a bounding box,
  // which we then want to represent as a graphic of a different
  // scale and origin, like a 1000x1000 pixel box whose bottom left is
  // 0,0 (call these "screen coordinates").
  //
  // This type is a transformation conveniently derived from the
  // bounds and desired screen coordinates. (It's called a scaler but
  // it also involves at least a translation.) The scaler is immutable,
  // even if the bounds it was derived from is modified.
  struct Scaler {
    // Maps a point from the original coordinates (i.e., what was inserted
    // into bounds) to screen coordinates.
    double ScaleX(double x) const;
    double ScaleY(double y) const;
    std::pair<double, double> Scale(double x, double y) const;
    std::pair<double, double> Scale(std::pair<double, double> p) const;

    // Inverse of the above (i.e., convert from screen coordinates to
    // points in the original coordinate system).
    double UnscaleX(double x) const;
    double UnscaleY(double y) const;
    std::pair<double, double> Unscale(double x, double y) const;
    std::pair<double, double> Unscale(std::pair<double, double> p) const;

    // Derive new scalers.

    // Flip the contents of the screen along the y axis.
    Scaler FlipY() const;
    // Shifts the display of the data so that a data point that formerly
    // displayed at screen coordinates (0,0) now appears at
    // (screenx, screeny).
    Scaler PanScreen(double screenx, double screeny) const;
    // Scale data in the x/y dimensions by the given factor (i.e. 1.0
    // does nothing).
    Scaler Zoom(double xfactor, double yfactor) const;

   private:
    friend struct Bounds;
    double xoff = 0.0, yoff = 0.0;
    double xs = 1.0, ys = 1.0;
    // In original coordinate system.
    double width = 0.0, height = 0.0;
  };

  // Make the bounding box as large as possible without modifying its
  // aspect ratio. If centered is set, center the data along the dimension
  // that is not filled.
  Scaler ScaleToFit(double w, double h, bool centered = true) const;
  // Make the bounding box fit the screen, stretching as necessary.
  Scaler Stretch(double w, double h) const;

  // TODO: log scale?

 private:
  double minx = std::numeric_limits<double>::infinity();
  double miny = std::numeric_limits<double>::infinity();
  double maxx = -std::numeric_limits<double>::infinity();
  double maxy = -std::numeric_limits<double>::infinity();
  bool is_empty_x = true, is_empty_y = true;
};

// Same, with integer coordinates.
struct IntBounds {
  // With no points.
  IntBounds();

  // Expand the bounding box to contain the point.
  void Bound(int64_t x, int64_t y);
  void Bound(std::pair<int64_t, int64_t> p);
  void BoundX(int64_t x);
  void BoundY(int64_t y);

  // The margins are included.
  bool Contains(int64_t x, int64_t y) const;

  // Returns true if we don't have points on both the x and y axes
  // (because Bound has not been called, or not both of BoundX and BoundY).
  // When the bounding box is empty, several functions below should not
  // be called.
  bool Empty() const;

  int64_t MinX() const;
  int64_t MinY() const;
  int64_t MaxX() const;
  int64_t MaxY() const;

  // The offset of the input point within the bounding box (thinking
  // of MinX/MinY as 0,0).
  int64_t OffsetX(int64_t x) const;
  int64_t OffsetY(int64_t y) const;

  int64_t Width() const;
  int64_t Height() const;

  // Modifies 'this'. 'this' and 'other' may be empty.
  void Union(const IntBounds &other);

  // Adds a fixed-sized margin around the entire bounds, in absolute units.
  // If empty, does nothing. d must be non-negative.
  void AddMargin(int64_t d);
  // Add margin to the sides of the bounding box.
  // If empty, does nothing. Arguments must be non-negative.
  void AddMargins(int64_t up, int64_t right, int64_t down, int64_t left);

  // Add a margin on both sides of each dimension that's a fraction
  // (e.g. 0.01 for 1% on each side) of its current width. If empty,
  // does nothing. Since the bounds are always integers, this may
  // do nothing if the margin rounds down to zero.
  // f must be non-negative.
  void AddMarginFrac(double f);
  void AddMarginsFrac(double fup, double fright,
                      double fdown, double fleft);

  // TODO: Scaling?

 private:
  int64_t minx = std::numeric_limits<std::int64_t>::max();
  int64_t miny = std::numeric_limits<std::int64_t>::max();
  int64_t maxx = std::numeric_limits<std::int64_t>::min();
  int64_t maxy = std::numeric_limits<std::int64_t>::min();
  bool is_empty_x = true, is_empty_y = true;
};

#endif
