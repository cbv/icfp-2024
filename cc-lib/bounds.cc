#include "bounds.h"

#include <algorithm>
#include <utility>
#include <cmath>

Bounds::Bounds() {}

void Bounds::Bound(double x, double y) {
  BoundX(x);
  BoundY(y);
}
void Bounds::Bound(std::pair<double, double> p) {
  BoundX(p.first);
  BoundY(p.second);
}
void Bounds::BoundX(double x) {
  if (std::isnan(x)) return;
  if (x < minx) minx = x;
  if (x > maxx) maxx = x;
  is_empty_x = false;
}
void Bounds::BoundY(double y) {
  if (std::isnan(y)) return;
  if (y < miny) miny = y;
  if (y > maxy) maxy = y;
  is_empty_y = false;
}

bool Bounds::Contains(double x, double y) const {
  if (std::isnan(x) || std::isnan(y)) return false;
  return x >= minx && x <= maxx &&
    y >= miny && y <= maxy && !(is_empty_x || is_empty_y);
}

bool Bounds::Empty() const { return is_empty_x || is_empty_y; }

double Bounds::MinX() const { return minx; }
double Bounds::MinY() const { return miny; }
double Bounds::MaxX() const { return maxx; }
double Bounds::MaxY() const { return maxy; }

double Bounds::OffsetX(double x) const { return x - minx; }
double Bounds::OffsetY(double y) const { return y - miny; }

double Bounds::Width() const { return OffsetX(MaxX()); }
double Bounds::Height() const { return OffsetY(MaxY()); }

void Bounds::Union(const Bounds &other) {
  if (other.Empty()) return;
  Bound(other.MinX(), other.MinY());
  Bound(other.MinX(), other.MaxY());
  Bound(other.MaxX(), other.MinY());
  Bound(other.MaxX(), other.MaxY());
}

void Bounds::AddMargin(double d) {
  AddMargins(d, d, d, d);
}

void Bounds::AddMargins(double up, double right, double down, double left) {
  if (Empty()) return;
  maxx += right;
  maxy += down;
  minx -= left;
  miny -= up;
}

void Bounds::AddMarginFrac(double f) {
  AddMarginsFrac(f, f, f, f);
}

void Bounds::AddMarginsFrac(double fup, double fright,
                            double fdown, double fleft) {
  if (Empty()) return;
  const double left = fleft * Width();
  const double right = fright * Width();
  const double up = fup * Height();
  const double down = fdown * Height();
  AddMargins(up, right, down, left);
}

double Bounds::Scaler::ScaleX(double x) const {
  return (x + xoff) * xs;
}
double Bounds::Scaler::ScaleY(double y) const {
  return (y + yoff) * ys;
}
std::pair<double, double>
Bounds::Scaler::Scale(double x, double y) const {
  return {ScaleX(x), ScaleY(y)};
}
std::pair<double, double>
Bounds::Scaler::Scale(std::pair<double, double> p) const {
  return {ScaleX(p.first), ScaleY(p.second)};
}

double Bounds::Scaler::UnscaleX(double x) const {
  // PERF could compute and save xs inverse?
  return (x / xs) - xoff;
}
double Bounds::Scaler::UnscaleY(double y) const {
  // PERF could compute and save xs inverse?
  return (y / ys) - yoff;
}
std::pair<double, double>
Bounds::Scaler::Unscale(double x, double y) const {
  return {UnscaleX(x), UnscaleY(y)};
}
std::pair<double, double>
Bounds::Scaler::Unscale(std::pair<double, double> p) const {
  return {UnscaleX(p.first), UnscaleY(p.second)};
}

Bounds::Scaler Bounds::ScaleToFit(double neww, double newh,
                                  bool centered) const {
  const double oldw = maxx - minx;
  const double oldh = maxy - miny;

  const double desired_xs = oldw == 0.0 ? 1.0 : (neww / oldw);
  const double desired_ys = oldh == 0.0 ? 1.0 : (newh / oldh);
  const double scale = std::min(desired_xs, desired_ys);

  // offset for centering, which is applied in the original coordinate
  // system.
  // XXX: This is mathematically correct, but at least one of these
  // should always be exactly zero; might be better to do this
  // by case analysis?
  const double center_x = centered ? (neww / scale - oldw) * 0.5 : 0.0;
  const double center_y = centered ? (newh / scale - oldh) * 0.5 : 0.0;

  Scaler ret;
  ret.xoff = center_x - minx;
  ret.yoff = center_y - miny;
  ret.xs = scale;
  ret.ys = scale;
  ret.width = oldw;
  ret.height = oldh;
  return ret;
}

Bounds::Scaler Bounds::Stretch(double neww, double newh) const {
  const double oldw = maxx - minx;
  const double oldh = maxy - miny;
  Scaler ret;
  ret.xoff = 0.0 - minx;
  ret.yoff = 0.0 - miny;
  ret.xs = oldw == 0.0 ? 1.0 : (neww / oldw);
  ret.ys = oldh == 0.0 ? 1.0 : (newh / oldh);
  ret.width = oldw;
  ret.height = oldh;
  return ret;
}

Bounds::Scaler Bounds::Scaler::FlipY() const {
  Scaler ret = *this;
  ret.ys = -ys;
  ret.yoff = yoff - height;
  return ret;
}

Bounds::Scaler Bounds::Scaler::PanScreen(double sx, double sy) const {
  Scaler ret = *this;
  double xo = sx / xs, yo = sy / ys;
  ret.xoff += xo;
  ret.yoff += yo;
  return ret;
}

Bounds::Scaler Bounds::Scaler::Zoom(double xfactor, double yfactor) const {
  Scaler ret = *this;
  // ret.xoff *= xfactor;
  // ret.yoff *= yfactor;
  ret.xs *= xfactor;
  ret.ys *= yfactor;
  ret.width *= xfactor;
  ret.height *= yfactor;
  return ret;
}



IntBounds::IntBounds() {}

void IntBounds::Bound(int64_t x, int64_t y) {
  BoundX(x);
  BoundY(y);
}
void IntBounds::Bound(std::pair<int64_t, int64_t> p) {
  BoundX(p.first);
  BoundY(p.second);
}
void IntBounds::BoundX(int64_t x) {
  if (x < minx) minx = x;
  if (x > maxx) maxx = x;
  is_empty_x = false;
}
void IntBounds::BoundY(int64_t y) {
  if (y < miny) miny = y;
  if (y > maxy) maxy = y;
  is_empty_y = false;
}

bool IntBounds::Contains(int64_t x, int64_t y) const {
  return x >= minx && x <= maxx &&
    y >= miny && y <= maxy && !(is_empty_x || is_empty_y);
}

bool IntBounds::Empty() const { return is_empty_x || is_empty_y; }

int64_t IntBounds::MinX() const { return minx; }
int64_t IntBounds::MinY() const { return miny; }
int64_t IntBounds::MaxX() const { return maxx; }
int64_t IntBounds::MaxY() const { return maxy; }

int64_t IntBounds::OffsetX(int64_t x) const { return x - minx; }
int64_t IntBounds::OffsetY(int64_t y) const { return y - miny; }

int64_t IntBounds::Width() const { return OffsetX(MaxX()); }
int64_t IntBounds::Height() const { return OffsetY(MaxY()); }

void IntBounds::Union(const IntBounds &other) {
  if (other.Empty()) return;
  Bound(other.MinX(), other.MinY());
  Bound(other.MinX(), other.MaxY());
  Bound(other.MaxX(), other.MinY());
  Bound(other.MaxX(), other.MaxY());
}


void IntBounds::AddMargin(int64_t d) {
  AddMargins(d, d, d, d);
}

void IntBounds::AddMargins(int64_t up, int64_t right,
                           int64_t down, int64_t left) {
  if (Empty()) return;
  maxx += right;
  maxy += down;
  minx -= left;
  miny -= up;
}

void IntBounds::AddMarginFrac(double f) {
  AddMarginsFrac(f, f, f, f);
}

void IntBounds::AddMarginsFrac(double fup, double fright,
                               double fdown, double fleft) {
  if (Empty()) return;
  const int64_t left = std::round(fleft * Width());
  const int64_t right = std::round(fright * Width());
  const int64_t up = std::round(fup * Height());
  const int64_t down = std::round(fdown * Height());
  AddMargins(up, right, down, left);
}
