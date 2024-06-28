
// Defines an interval cover, which is a sparse set of non-overlapping
// spans:
//
//   - Spans are defined as [start, end) as elsewhere.
//   - The indices are uint64.
//   - All we store about an interval is the data it contains.
//     If two adjacent intervals have equal data, they are merged.
//   - Sparse meaning that the cover describes the entire space
//     from 0 to 2^64 - 1, including explicit gaps.
//     TODO: More careful about how this ends. We treat 2^64 - 1
//     as an invalid start; it should be the end of the final interval.
//   - Takes space proportional to the number of intervals.
//   - Most operations are logarithmic in the number of intervals.
//
// TODO: Interval-KD-cover for multidimensional data?
//
// Value semantics.
//
// Not thread safe; clients should manage mutual exclusion.

#ifndef _CC_LIB_INTERVAL_COVER_H
#define _CC_LIB_INTERVAL_COVER_H

#include <optional>
#include <map>
#include <cstdint>
#include <cstdio>

#include "base/logging.h"

// Data must have value semantics and is copied willy-nilly. It must
// also have an equivalence relation ==. If it's some simple data like
// int or bool, you're all set.
//
// Two adjacent intervals (no gap) will always have different Data
// values, and the operations below automatically merge adjacent
// intevals to ensure this. Empty intervals (start == end) are
// discarded, since they span no points.
template<class D>
struct IntervalCover {
  using Data = D;
  struct Span {
    uint64_t start;
    // One after the end.
    uint64_t end;
    Data data;
    Span(uint64_t s, uint64_t e, Data d) : start(s), end(e), data(d) {}
  };

  // Create an empty interval cover, where the entire range is
  // the default value.
  explicit IntervalCover(Data def);

  // Get the span that covers the specific point. There is always
  // exactly one such span.
  Span GetPoint(uint64_t pt) const;
  const Data &operator[] (size_t idx) const;

  // Split at the point. The right-hand-side of the split gets
  // the supplied data, and the left-hand retains the existing
  // data. (Note that splitting can produce empty intervals,
  // which are then eliminated, or if rhs equals the existing
  // data, the split has no effect because the pieces are
  // instantly merged together again.)
  //
  // This is a good way to update the data in an interval, by
  // passing pt equal to its start. It handles any merging.
  void SplitRight(uint64_t pt, Data rhs);

  // Set the supplied span, overwriting anything underneath.
  // Might still merge with adjacent intervals, of course.
  void SetSpan(uint64_t start, uint64_t end, Data d);
  void SetPoint(uint64_t pt, Data d) { SetSpan(pt, pt + 1, d); }

  // Get the start of the very first span, which is always 0.
  constexpr uint64_t First() const;

  // Returns true if this point starts after any interval.
  // This is just true for 2^64 - 1.
  constexpr bool IsAfterLast(uint64_t pt) const;

  // Get the start of the span that starts strictly after this
  // point. Should not be called for a point p where IsAfterLast(p)
  // is true.
  // To iterate through spans,
  //
  //   for (uint64_t pt = ic.First();
  //        !ic.IsAfterLast(pt);
  //        pt = ic.Next(pt)) { ... }
  //
  // TODO: Make iterator version, probably for ranges? It
  // can be more efficient, too, if we keep the underlying
  // map iterator.
  uint64_t Next(uint64_t pt) const;

  // Get the start of the span that starts strictly before this
  // point. Should not be called for a point in the first span
  // (i.e. where GetPoint(pt).start = 0).
  //
  //   for (uint64_t pt = ic.GetPoint(x).start;
  //        pt != 0;
  //        pt = ic.Prev(pt)) { ... }
  uint64_t Prev(uint64_t pt) const;


  // XXX should be like "IntersectsWith"
  template<class Other>
  static bool IntersectWith(const Span &mine,
                            const typename IntervalCover<Other>::Span &theirs,
                            uint64_t *start, uint64_t *end);

  // Verifies internal invariants and aborts if they are violated.
  // Typically only useful for debugging.
  void CheckInvariants() const;
  // Won't show data unless it's of a limited number of basic types.
  void DebugPrint() const;

 private:
  static constexpr uint64_t MAX64 = (uint64_t)-1;
  // Make a Span by looking at the next entry in the map to
  // compute the end. The iterator may not denote spans.end().
  Span MakeSpan(typename std::map<uint64_t, Data>::const_iterator &it) const;

  typename std::map<uint64_t, Data>::iterator
  GetInterval(uint64_t pt);
  typename std::map<uint64_t, Data>::const_iterator
  GetInterval(uint64_t pt) const;

  // Split at the point. Returns the span to the left, and the span to
  // the right. If the split already exists; returns the existing spans.
  // If 0, then the left component will be nullopt; if MAX64, the right
  // will be nullopt. Caller must restore invariants, like these two spans
  // may not have the same data!
  std::pair<std::optional<typename std::map<uint64_t, D>::iterator>,
            std::optional<typename std::map<uint64_t, D>::iterator>>
  SplitAt(uint64_t pt);


  // Actual representation is an stl map with some invariants.
  std::map<uint64_t, Data> spans;
};

// Template implementations follow.

template<class D>
template<class O>
bool IntervalCover<D>::IntersectWith(
    const Span &mine,
    const typename IntervalCover<O>::Span &theirs,
    uint64_t *start, uint64_t *end) {
  // First, make sure they actually intersect.
  if (mine.end <= theirs.start ||
      theirs.end <= mine.start) {
    return false;
  } else {
    // We know they overlap. Intersection is the max of the starts
    // and the min of the ends, then.
    *start = max(mine.start, theirs.start);
    *end = min(mine.end, theirs.end);
    return true;
    // Verify various cases visually:

    // mine    ---------
    // theirs ----

    // mine -----------
    // theirs      --------

    // mine -----------
    // theirs  ----
  }
}

template<class D>
auto IntervalCover<D>::GetInterval(uint64_t pt) ->
    typename std::map<uint64_t, D>::iterator {
  // Find the interval that 'start' is within. We'll modify this
  // interval.
  auto it = spans.upper_bound(pt);
  // Impossible; first interval starts at 0, the lowest uint64.
  CHECK(it != spans.begin());
  --it;
  return it;
}

// Same, const.
template<class D>
auto IntervalCover<D>::GetInterval(uint64_t pt) const ->
  typename std::map<uint64_t, D>::const_iterator {
  auto it = spans.upper_bound(pt);
  CHECK(it != spans.begin());
  --it;
  return it;
}

template<class D>
IntervalCover<D>::IntervalCover(D d) : spans { { 0ULL, d } } {}

template<class D>
constexpr uint64_t IntervalCover<D>::First() const {
  return 0ULL;
}

template<class D>
uint64_t IntervalCover<D>::Next(uint64_t pt) const {
  auto it = spans.upper_bound(pt);
  if (it == spans.end()) return MAX64;
  else return it->first;
}

template<class D>
uint64_t IntervalCover<D>::Prev(uint64_t pt) const {
  // Gets the first span that starts strictly after the point.
  typename std::map<uint64_t, D>::const_iterator it = spans.upper_bound(pt);
  // Should never happen, because the first interval starts
  // at 0.
  CHECK(it != spans.begin());
  --it;
  // Now we want the interval before that.
  CHECK(it != spans.begin()) << "Prev() called on the first interval.";
  --it;
  return it->first;
}

template<class D>
constexpr bool IntervalCover<D>::IsAfterLast(uint64_t pt) const {
  return pt == MAX64;
}

template<class D>
auto IntervalCover<D>::MakeSpan(
    typename std::map<uint64_t, D>::const_iterator &it) const -> Span {
  typename std::map<uint64_t, D>::const_iterator next = std::next(it);
  if (next == spans.end()) {
    return Span(it->first, MAX64, it->second);
  } else {
    return Span(it->first, next->first, it->second);
  }
}

template<class D>
auto IntervalCover<D>::GetPoint(uint64_t pt) const -> Span {
  // Gets the first span that starts strictly after the point.
  typename std::map<uint64_t, D>::const_iterator it = spans.upper_bound(pt);
  // Should never happen, because the first interval starts
  // at LLONG_MIN.
  CHECK(it != spans.begin());
  --it;
  return MakeSpan(it);
}

template<class D>
auto IntervalCover<D>::operator [](size_t idx) const -> const Data & {
  // Just like GetPoint, but get a reference to just the data.
  typename std::map<uint64_t, D>::const_iterator it = spans.upper_bound(idx);
  // Should never happen, because the first interval starts
  // at LLONG_MIN.
  CHECK(it != spans.begin());
  --it;
  return it->second;
}

template<class D>
void IntervalCover<D>::SetSpan(uint64_t start, uint64_t end, D new_data) {
  // printf("SetSpan %llu %llu ...\n", start, end);

  // Interval must be legal.
  CHECK(start <= end);

  // Don't do anything for empty intervals.
  if (start == end)
    return;

  // This should be impossible because start is strictly less
  // than some uint64_t.
  CHECK(!this->IsAfterLast(start));

  // First, make sure that any overlapping at the ends is OK.
  auto [lao, rao] = SplitAt(start);
  auto [lbo, rbo] = SplitAt(end);

  if (rao.has_value()) rao.value()->second = new_data;
  if (lbo.has_value()) lbo.value()->second = new_data;

  // Now, for any intervals contained completely within this one,
  // set them to the target data. This also restores invariants
  // by merging intervals with the same data.
  for (uint64_t pos = start; pos < end; pos = Next(pos)) {
    // This can't go off the end because pos is less than some
    // uint64_t, and only MAX64 is after last.
    CHECK(!this->IsAfterLast(pos));

    Span span = GetPoint(pos);
    CHECK(span.start >= start);
    if (span.end <= end) {
      /*
      printf("Set fully-contained interval [%llu,%llu).\n",
             span.start, span.end);
      */
      SplitRight(span.start, new_data);
    }
  }
}

template<class D>
std::pair<std::optional<typename std::map<uint64_t, D>::iterator>,
          std::optional<typename std::map<uint64_t, D>::iterator>>
IntervalCover<D>::SplitAt(uint64_t pt) {
  if (pt == 0) {
    return std::make_pair(std::nullopt, std::make_optional(spans.begin()));
  } else if (pt == MAX64) {
    auto it = spans.end();
    --it;
    return std::make_pair(std::make_optional(it), std::nullopt);
  }

  auto it = spans.upper_bound(pt);
  // Impossible; first interval starts at 0, the lowest uint64.
  CHECK(it != spans.begin());
  --it;

  // Existing interval?
  if (it->first == pt) {
    // Impossible; we already checked!
    CHECK(pt != 0 && it != spans.begin());
    auto it2 = it;
    --it;
    return std::make_pair(it, it2);
  }

  // Not an existing interval. Create a new one with the same
  // data (an illegal state!).
  auto [it2, success] = spans.insert(std::make_pair(pt, it->second));
  CHECK(success);
  return std::make_pair(it, it2);
}

// XXX maybe can use SplitAt for this?
template<class D>
void IntervalCover<D>::SplitRight(uint64_t pt, D rhs) {
  // printf("Splitright %llu\n", pt);
  // Find the interval in which pt lies.
  auto it = GetInterval(pt);

  // Avoid doing any work if we're splitting but would have to
  // merge back together immediately.
  // if (it->second == rhs)
  // return;

  // And if we're asking to split the interval exactly on
  // its start point, just replace the data.
  if (it->first == pt) {
    // printf("it's update\n");
    // But do we need to merge left?
    if (it != spans.begin()) {

      auto prev = std::prev(it);
      if (prev->second == rhs) {
        // printf("merge left\n");
        // If merging left, we delete this interval, but then
        // 'it' becomes the merged interval so we can continue
        // merging right below.
        spans.erase(it);
        it = prev;
      }

      /*
        // If merging left, then just delete this interval.
        auto nextnext = spans.erase(it);
        // But now maybe prev and nextnext need to be merged?
        if (nextnext != spans.end() &&
            prev->second == nextnext->second) {
          // But this is as far as we'd need to merge, because
          // if next(nextnext) ALSO had the same data, then
          // it would have the same data as the adjacent nextnext,
          // which is illegal.
          (void)spans.erase(nextnext);
        }
        // Anyway, we're done.
        return;
      */
    }

    CHECK(it != spans.end());

    auto next = std::next(it);
    // if (next != spans.end())
    // printf("Next is %llu\n", next->first);
    while (next != spans.end() && next->second == rhs) {
      auto nextnext = std::next(next);
      // printf("Delete %llu.\n", next->first);
      // Then just delete the next one, making it part of
      // this one.
      (void)spans.erase(next);
      next = nextnext;
    }

    // But do continue to set the data either way...
    it->second = rhs;
    return;
  }

  // printf("(not update\n");
  // If there's a next interval and the new data is the same
  // as the next, then another simplification:
  auto next = std::next(it);
  if (next != spans.end() && next->second == rhs) {
    // This would be a bug. Make sure we're moving the next's start
    // to the left. We can't be creating an empty interval because
    // we already ensured that it->first < pt.
    CHECK(pt < next->first);
    // We can't actually modify the key in std::map (we'd like to
    // set it to pt, which would maintain the order invariant),
    // but erasing and inserting with a hint is amortized constant
    // time:
    auto nextnext = spans.erase(next);
    spans.insert(nextnext, std::make_pair(pt, rhs));
    return;
  }

  // In this case, we just need to insert a new interval, which
  // is guaranteed to be non-empty and have distinct data to
  // the left and right.
  //
  // In C++11, hint is the element *following* where this one
  // would go.
  spans.insert(next, std::make_pair(pt, rhs));
}

// When the data are strings, specialize debugprint (used mainly
// in tests.) The specializations are complete definitions so they
// go in the .cc file.
template<>
void IntervalCover<std::string>::DebugPrint() const;
template<>
void IntervalCover<int>::DebugPrint() const;
template<>
void IntervalCover<bool>::DebugPrint() const;

template<class D>
void IntervalCover<D>::DebugPrint() const {
  printf("------\n");
  for (const auto &p : spans) {
    printf("%llu: ?\n", p.first);
  }
  printf("------\n");
}

template<class D>
void IntervalCover<D>::CheckInvariants() const {
  CHECK(!spans.empty());
  CHECK(spans.begin()->first == 0);
  auto it = spans.begin();
  uint64_t prev = it->first;
  D prev_data = it->second;
  ++it;
  for (; it != spans.end(); ++it) {
    // (should be guaranteed by map itself)
    CHECK(prev < it->first);
    CHECK(!(prev_data == it->second));
    prev = it->first;
    prev_data = it->second;
  }
}

#endif
