
#ifndef _CC_LIB_GEOM_BEZIER_H
#define _CC_LIB_GEOM_BEZIER_H

#include <tuple>

// Returns {closest_x, closest_y, dist}.
std::tuple<float, float, float>
DistanceFromPointToQuadBezier(
    // The point to test
    float px, float py,
    // Bezier start point
    float sx, float sy,
    // Bezier ontrol point
    float cx, float cy,
    // Bezier end point
    float ex, float ey);

#endif
