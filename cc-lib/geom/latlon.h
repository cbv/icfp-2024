
#ifndef _CC_LIB_LATLON_H
#define _CC_LIB_LATLON_H

#include <utility>
#include <optional>
#include <functional>
#include <string>

// Position on the earth.
struct LatLon {
  // For vectors, etc.
  LatLon() : lat(0.0), lon(0.0) {}
  LatLon(const LatLon &other) : lat(other.lat), lon(other.lon) {}

  /* latitude measured in degrees. Positive is north and negative is south.
     longitude measured in degrees. Positive is east and negative is west.

     Careful: lat,lon is like y,x coordinates.

     The inputs are treated modularly, so that +100 latitude is the
     same as -80. */
  static LatLon FromDegs(double lat, double lon);

  /* The string must be of the form lat,lon where each component
     is specified in decimal degrees, like 40.440624,-79.995888.
     Negative values should be used for S and W hemispheres.

     This function may understand more formats in the future. */
  static std::optional<LatLon> FromString(const std::string &s);


  // TODO: FromDMS, FromRads, etc.

  /* Returns (lat, lon).
     latitude always in [-90, +90),
     longitude always in [-180, +180). */
  std::pair<double, double> ToDegs() const;
  // TODO: ToRads

  /* Angle of a vector from a to b, in radians, where 0 is to the East
     and pi/2 is to the north. The angle is undefined when the points
     are the same or antipodes, in which case nullopt is returned.
     nb. I don't think this works if the source is the North or South
     pole. */
  static std::optional<double> Angle(LatLon a, LatLon b);


  // Great circle distances between positions.
  static double DistMeters(LatLon a, LatLon b);
  static double DistFeet(LatLon a, LatLon b);
  static double DistMiles(LatLon a, LatLon b);
  static double DistNauticalMiles(LatLon a, LatLon b);
  static double DistKm(LatLon a, LatLon b);

  /* A projection maps positions to x,y coordinates. Many projections
     have infinite extent along one dimension, so we also specify the
     ranges of the output coordinates. */
  using Projection = std::function<std::pair<double, double>(LatLon)>;
  // Mapping (x,y) coordinates to positions on Earth
  using InverseProjection = std::function<LatLon(double, double)>;

  /* Produces x coordinates [-pi to +pi] and y coordinates -inf to +inf.
     Mercator distorts area near the poles badly.
     Give the meridian that shall be the longitudinal center of the map,
     in degrees. */
  static Projection Mercator(double meridian_degs);

  // Uses the prime meridian as the longitudinal center of the map.
  static Projection PrimeMercator();

  /* Very simple projection where meridians and parallels are equally
     spaced straight lines. Does not preserve much of anything, but decent
     for local maps. Argument is the parallel (in degrees) where the scale
     is not distorted. For local maps, set this to a parallel within that
     region.
     Range is x: [-pi to +pi] and y: [-pi/2 to +pi/2].
  */
  static Projection Equirectangular(double parallel_degs);

  // Equirectangular when the equator is the standard parallel.
  static Projection PlateCarree();

  /* Gnomonic (rectilinear) projection centered on the given tangent point.
     This projects both hemispheres atop one another (the center point and
     its antipode will both be projected to 0,0). The map is highly distorted
     in area except near the tangent point and its antipode, but all great
     circles are straight lines.
     Range is x: [-inf to +inf] and y: [-inf to +inf].
  */
  static Projection Gnomonic(LatLon tangent);
  // Inverse may not be accurate for large distances, but seems good within
  // a few hundred miles of the tangent point.
  static InverseProjection InverseGnomonic(LatLon tangent);

  // Project coordinates to a rectangle with linear scaling, as though
  // the Earth is flat. Does not handle the antimeridan. A reasonable
  // option for plotting onto map tiles at city-level zooms where the
  // surface is approximately flat anywhere.
  static Projection Linear(LatLon zerozero, LatLon oneone);
  static InverseProjection InverseLinear(LatLon zerozero, LatLon oneone);

private:
  LatLon(double lat, double lon) : lat(lat), lon(lon) {}
  // Internally stored as degrees, although rads might be better.
  double lat = 0.0;
  double lon = 0.0;
};

#endif
