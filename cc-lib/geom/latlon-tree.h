
#ifndef _CC_LIB_GEOM_LATLON_TREE_H
#define _CC_LIB_GEOM_LATLON_TREE_H

#include <memory>
#include <variant>
#include <utility>
#include <vector>
#include <tuple>

#include "geom/latlon.h"
#include "base/logging.h"

namespace internal { template<class T> struct LLKDTree; }

// Maps LatLon to some data T (value semantics).
// Allows querying for points close to a given position.
//
// Does not treat poles correctly (in the sense that close points
// may not be returned if the search radius includes a pole), but
// this could be fixed by further segmenting the KD trees for points
// near the poles.
//
// This uses a 2D-tree internally so it is asymptotically decent,
// but it is not particularly fast:
//   - It uses the iterative inverse Vincenty to compute distances
//   - It does not choose the best split axes (uses mean)
//   - It never rebalances, so it is sensitive to insertion order
//   - It ought to be smarter about reducing the search radius
//     when crossing splits.
template<class T>
struct LatLonTree {

  void Insert(LatLon pos, T t);

  template<class F>
  void App(const F &f) const;

  // Return all points within the given radius, with their data
  // and the distance (in meters).
  std::vector<std::tuple<LatLon, T, double>>
  Lookup(LatLon pos, double radius) const;

  // TODO: Closest point query.

 private:
  internal::LLKDTree<T> east, west;
};


// Implementations follow.

namespace internal {
template<class T>
struct LLKDTree {
  static constexpr size_t MAX_LEAF = 8;
  using Leaf =
    std::vector<std::pair<LatLon, T>>;
  // struct Split;
  struct Split {
    // Whether we are splitting on the lat dimension
    // or lon dimension.
    bool lat = false;
    // The location of the split.
    double axis = 0.0;
    std::unique_ptr<std::variant<Leaf, Split>> lesseq, greater;
  };
  using Node = std::variant<Leaf, Split>;

  LLKDTree() {
    // An empty leaf.
    root = std::make_unique<Node>(std::in_place_type<Leaf>);
  }

  static inline bool Classify(LatLon pos, bool is_lat, double axis) {
    const auto [lat, lon] = pos.ToDegs();
    if (is_lat) {
      return lat <= axis;
    } else {
      return lon <= axis;
    }
  }

  void Insert(LatLon pos, T t) {
    // Arbitrary preference for first split we create.
    InsertTo(root.get(), false, pos, t);
  }

  // Calls f(latlon, t) for every point in the tree in
  // arbitrary order. The callback should not modify an
  // alias of the tree!
  template<class F>
  void App(const F &f) const {
    std::vector<Node *> q = {root.get()};
    while (!q.empty()) {
      Node *node = q.back();
      q.pop_back();

      if (Split *split = std::get_if<Split>(node)) {
        if (split->lesseq.get() != nullptr)
          q.push_back(split->lesseq.get());
        if (split->greater.get() != nullptr)
          q.push_back(split->greater.get());

      } else {
        for (const auto &[ll, t] : std::get<Leaf>(*node)) {
          f(ll, t);
        }
      }
    }
  }

  std::vector<std::tuple<LatLon, T, double>>
  Lookup(LatLon pos, double radius) const {
    const auto &[pos_lat, pos_lon] = pos.ToDegs();
    // Because of the radius, we have to look on both sides of the
    // split sometimes.
    // In principle we could also reduce the search radius when
    // we cross an axis; this is easy when using Euclidean distance
    // but I'd need to check that it is actually correct for
    // great circles.

    std::vector<Node *> q = {root.get()};
    std::vector<std::tuple<LatLon, T, double>> out;
    while (!q.empty()) {
      Node *node = q.back();
      q.pop_back();

      if (Split *split = std::get_if<Split>(node)) {

        // Project the lookup point to the split axis.
        // PERF: There should be a faster distance than Vincenty
        // since we know they share a latitude or longitude.
        const LatLon axispt =
          split->lat ? LatLon::FromDegs(split->axis, pos_lon) :
          LatLon::FromDegs(pos_lat, split->axis);

        // If it is within the radius, we need to check both.
        const double axisdist = LatLon::DistMeters(pos, axispt);
        const bool both = axisdist <= radius;
        const bool lesseq = Classify(pos, split->lat, split->axis);

        if (both || lesseq) {
          if (split->lesseq.get() != nullptr) {
            q.push_back(split->lesseq.get());
          }
        }

        if (both || !lesseq) {
          if (split->greater.get() != nullptr) {
            q.push_back(split->greater.get());
          }
        }

      } else {
        for (const auto &[ll, t] : std::get<Leaf>(*node)) {
          const double dist = LatLon::DistMeters(pos, ll);
          if (dist <= radius) {
            out.emplace_back(ll, t, dist);
          }
        }
      }
    }

    return out;
  }

 private:
  void InsertTo(Node *node, bool use_lat, LatLon pos, T t) {
    for (;;) {
      CHECK(node != nullptr);
      if (Split *split = std::get_if<Split>(node)) {
        // Next split should be the other axis.
        use_lat = !split->lat;
        bool lesseq = Classify(pos, split->lat, split->axis);
        // Leaves are created lazily, so add an empty one if
        // null (but do the actual insertion on the next pass).
        if (lesseq) {
          if (split->lesseq.get() == nullptr) {
            split->lesseq =
              std::make_unique<Node>(std::in_place_type<Leaf>);
          }
          node = split->lesseq.get();
        } else {
          if (split->greater.get() == nullptr) {
            split->greater =
              std::make_unique<Node>(std::in_place_type<Leaf>);
          }
          node = split->greater.get();
        }
      } else {
        Leaf *leaf = &std::get<Leaf>(*node);
        leaf->emplace_back(pos, t);

        // May have exceeded max leaf size.
        const size_t num = leaf->size();
        if (num > MAX_LEAF) {

          // We'll replace the node in place, but we'll need
          // the old leaves to do it.
          std::vector<std::pair<LatLon, T>> old =
            std::move(*leaf);

          // PERF: Median is likely a better choice.
          double avg = 0.0;
          for (const auto &[ll, t_] : old) {
            const auto [lat, lon] = ll.ToDegs();
            avg += use_lat ? lat : lon;
          }
          avg /= num;

          CHECK(!std::holds_alternative<Split>(*node));

          // Replace contents of the node with a Split.
          node->template emplace<Split>(use_lat, avg, nullptr, nullptr);

          CHECK(std::holds_alternative<Split>(*node));

          // Now insert the old contents.
          for (const auto &[ll, t] : old) {
            InsertTo(node, use_lat, ll, t);
          }
        }
        // Inserted, so we are done.
        return;
      }
    }
  }

  /*

  (* Gets a single point with minimum distance (there may be many points
     that are equidistant). Only fails if the tree is empty.
     NOTE: Currently, linear time! *)
  val closestpoint : 'a tree -> pos -> ('a * dist) option

  */

  std::unique_ptr<Node> root;
};
}  // namespace

template<class T>
template<class F>
void LatLonTree<T>::App(const F &f) const {
  east.App(f);
  west.App(f);
}

template<class T>
void LatLonTree<T>::Insert(LatLon pos, T t) {
  const auto [lat, lon] = pos.ToDegs();
  if (lon < 0.0) {
    west.Insert(pos, t);
  } else {
    east.Insert(pos, t);
  }
}

template<class T>
std::vector<std::tuple<LatLon, T, double>>
LatLonTree<T>::Lookup(LatLon pos, double radius) const {
  const auto &[pos_lat, pos_lon] = pos.ToDegs();

  // If it's near longitude 0.0 or 180.0, we need to check both.
  if (LatLon::DistMeters(LatLon::FromDegs(pos_lat, 0.0), pos) <= radius ||
      LatLon::DistMeters(LatLon::FromDegs(pos_lat, 180.0), pos) <= radius) {
    // Slow case
    auto v1 = west.Lookup(pos, radius);
    auto v2 = east.Lookup(pos, radius);
    v1.reserve(v1.size() + v2.size());
    for (const auto &v : v2) v1.push_back(v);
    return v1;
  } else {
    return pos_lon < 0.0 ? west.Lookup(pos, radius) :
      east.Lookup(pos, radius);
  }
}


#endif
