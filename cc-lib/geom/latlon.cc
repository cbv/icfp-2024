
#include "geom/latlon.h"

#include <utility>
#include <optional>
#include <functional>
#include <string>
#include <cmath>
#include <numbers>

using namespace std;

inline constexpr double PI = std::numbers::pi;
inline constexpr double TWO_PI = 2.0 * PI;

// For distance calculations, I only ported the iterative solution from
// sml-lib, based on the note there.

// Return the unique x = d + r*max
// where x is in [-max, +max) and r is some integer.
// Used to put degrees in the range [-90, +90), for example, since
// some computations cause them to "wrap around".

// XXX As I recall, remainder (or fmod) can actually give you
// the endpoint of the expected open interval, because of
// floating point rounding up (precision is higher near zero
// than at 180). Might be a non-issue the way we use it,
// since the range is symmetric? Find a test case and fix.
// (Calling fmod twice may be the simplest.)
static double PlusMinusMod(double d, double mx) {
  // Shift by mx so that we have a value in [0, 2 * max).
  double twomax = mx * 2.0;
  double z = d + mx;
  double r = remainder(z, twomax);
  // but remainder can still return negative for negative z
  if (r < 0.0) r += twomax;
  // And return to centering around 0.
  return r - mx;
}

LatLon LatLon::FromDegs(double lat, double lon) {
  return LatLon(PlusMinusMod(lat, 90.0),
                PlusMinusMod(lon, 180.0));
}

static constexpr double RAD_TO_DEGS = 180.0 / PI;
static constexpr double DEGS_TO_RAD = PI / 180.0;

// was fromlargerads
static LatLon FromRads(double lat, double lon) {
  return LatLon::FromDegs(RAD_TO_DEGS * lat, RAD_TO_DEGS * lon);
}

std::pair<double, double> LatLon::ToDegs() const {
  return std::make_pair(lat, lon);
}

optional<LatLon> LatLon::FromString(const string &s_orig) {
  string s = s_orig;
  int p = s.find(',');
  if (p == string::npos) {
    return nullopt;
  }
  s[p] = 0;
  double lat, lon;
  {
    char *endptr = nullptr;
    lat = strtod(s.c_str(), &endptr);
    if (endptr == s.c_str() || endptr != s.c_str() + p) {
      return nullopt;
    }
  }
  {
    char *endptr = nullptr;
    lon = strtod(s.c_str() + p + 1, &endptr);
    if (endptr == s.c_str() + p + 1 ||
        endptr != s.c_str() + s.size()) {
      return nullopt;
    }
  }
  return LatLon(lat, lon);
}

// Originally based on Chris Veness's JavaScript LGPL implementation
// of the Vincenty inverse solution on an ellipsoid model of the Earth.
// ((c) Chris Veness 2002-2008)
// http://www.movable-type.co.uk/scripts/latlong-vincenty.html


static double DistMetersVincenty(LatLon pos1, LatLon pos2) {
  const auto [lat1, lon1] = pos1.ToDegs();
  const auto [lat2, lon2] = pos2.ToDegs();

  auto ToRad = [](double d) { return d * DEGS_TO_RAD; };

  const double a = 6378137.0;
  const double b = 6356752.3142;
  const double f = 1.0 / 298.257223563;
  const double L = ToRad(lon2 - lon1);
  const double U1 = atan((1.0 - f) * tan(ToRad(lat1)));
  const double U2 = atan((1.0 - f) * tan(ToRad(lat2)));
  const double sinU1 = sin(U1);
  const double cosU1 = cos(U1);
  const double sinU2 = sin(U2);
  const double cosU2 = cos(U2);

  double lambda = L;
  for (int iters_left = 20; iters_left > 0; iters_left--) {
    double sinLambda = sin(lambda);
    double cosLambda = cos(lambda);
    double sinSigma = sqrt((cosU2 * sinLambda) * (cosU2 * sinLambda) +
                           (cosU1 * sinU2 - sinU1 * cosU2 * cosLambda) *
                           (cosU1 * sinU2 - sinU1 * cosU2 * cosLambda));

    if (sinSigma == 0.0) return 0.0;

    double cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
    double sigma = atan2(sinSigma, cosSigma);
    double sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
    double cosSqAlpha = 1.0 - sinAlpha * sinAlpha;
    double cos2SigmaM = cosSigma - 2.0 * sinU1 * sinU2 / cosSqAlpha;
    cos2SigmaM = std::isnan(cos2SigmaM) ? 0.0 : cos2SigmaM;

    double C = f / 16.0 * cosSqAlpha * (4.0 + f * (4.0 - 3.0 * cosSqAlpha));
    double lambdaP = lambda;
    lambda = L + (1.0 - C) * f * sinAlpha *
      (sigma + C * sinSigma *
       (cos2SigmaM + C * cosSigma *
        (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));

    if (fabs(lambda - lambdaP) < 1e-12) {
      // Converged, so compute the final result.
      const double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
      const double A = 1.0 + uSq / 16384.0 *
        (4096.0 + uSq * (-768.0 + uSq * (320.0 - 175.0 * uSq)));

      const double B = uSq / 1024.0 *
        (256.0 + uSq * (-128.0 + uSq * (74.0 - 47.0 * uSq)));

      const double deltaSigma = B * sinSigma *
        (cos2SigmaM + B / 4.0 *
         (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM) -
          B / 6.0 * cos2SigmaM * (-3.0 + 4.0 * sinSigma * sinSigma) *
          (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));

      return b * A * (sigma - deltaSigma);
    }
  }

  // Didn't converge; returning NaN.
  return 0.0 / 0.0;
}

double LatLon::DistMeters(LatLon a, LatLon b) {
  return DistMetersVincenty(a, b);
}
double LatLon::DistKm(LatLon a, LatLon b) {
  return DistMeters(a, b) * 0.001;
}
double LatLon::DistMiles(LatLon a, LatLon b) {
  return DistKm(a, b) * 0.621371192;
}
double LatLon::DistFeet(LatLon a, LatLon b) {
  return DistMetersVincenty(a, b) * (1.0 / 0.3048);
}
double LatLon::DistNauticalMiles(LatLon a, LatLon b) {
  return DistKm(a, b) * 0.539956803;
}

LatLon::Projection LatLon::Mercator(double lambda0) {
  return [lambda0](LatLon pos) -> pair<double, double> {
    const auto [phi, lambda1] = pos.ToDegs();
      // Shift longitude to arrange for new central meridian, and make
      // sure the result has range ~180.0--180.0.
      double lambda_deg = PlusMinusMod(lambda1 - lambda0, 180.0);
      // Convert from degrees to radians.
      double lambda_rad = lambda_deg * DEGS_TO_RAD;
      // Note, sml code seemed to have a bug here?
      double phi_rad = phi * DEGS_TO_RAD;

      double sinphi = sin(phi_rad);

      return make_pair(lambda_rad, 0.5 * log((1.0 + sinphi) / (1.0 - sinphi)));
    };
}

LatLon::Projection LatLon::PrimeMercator() {
  return Mercator(0.0);
}

LatLon::Projection LatLon::Equirectangular(double phi1) {
  const double cosphi1 = cos(phi1 * DEGS_TO_RAD);
  return [cosphi1](LatLon pos) {
      const auto [phi, lambda] = pos;
      return make_pair(DEGS_TO_RAD * lambda * cosphi1, DEGS_TO_RAD * phi);
    };
}

LatLon::Projection LatLon::PlateCarree() {
  // Same as Equirectangluar(0), but saves the degenerate cos/multiply.
  return [](LatLon pos) {
      const auto [phi, lambda] = pos.ToDegs();
      return make_pair(DEGS_TO_RAD * lambda, DEGS_TO_RAD * phi);
    };
}

// http://mathworld.wolfram.com/GnomonicProjection.html
LatLon::Projection LatLon::Gnomonic(LatLon pos0) {
  const auto [phi1d, lambda0d] = pos0.ToDegs();
  const double phi1 = DEGS_TO_RAD * phi1d;
  const double lambda0 = DEGS_TO_RAD * lambda0d;
  const double sin_phi1 = sin(phi1);
  const double cos_phi1 = cos(phi1);
  return [lambda0, sin_phi1, cos_phi1](LatLon pos) {
      const auto [phid, lambdad] = pos.ToDegs();
      double phi = DEGS_TO_RAD * phid;
      double lambda = DEGS_TO_RAD * lambdad;
      double sin_phi = sin(phi);
      double cos_phi = cos(phi);
      double cos_lambda_minus_lambda0 = cos(lambda - lambda0);
      double cosc = sin_phi1 * sin_phi +
        cos_phi1 * cos_phi * cos_lambda_minus_lambda0;
      return make_pair((cos_phi * sin(lambda - lambda0)) / cosc,
         (cos_phi1 * sin_phi -
          sin_phi1 * cos_phi * cos_lambda_minus_lambda0) / cosc);
    };
}

LatLon::InverseProjection LatLon::InverseGnomonic(LatLon pos0) {
  const auto [phi1d, lambda0d] = pos0.ToDegs();
  double phi1 = DEGS_TO_RAD * phi1d;
  double lambda0 = DEGS_TO_RAD * lambda0d;
  double sin_phi1 = sin(phi1);
  double cos_phi1 = cos(phi1);

  return [lambda0, sin_phi1, cos_phi1](double x, double y) {
      double rho = sqrt(x * x + y * y);
      // XXX use atan2? wolfram says so. y, x?
      double c = atan(rho);
      double cosc = cos(c);
      double sinc = sin(c);

      double phi = asin(cosc * sin_phi1 +
                        /* In the case that the point being inverted
                           is exactly (0,0), rho will be zero here,
                           and the quantity undefined. */
                        (y == 0.0 ? 0.0 : (y * sinc * cos_phi1) / rho));

      // Same problem here. tan(0) is 0 so just don't compute it
      // in that branch.
      double xdist =
        x == 0.0 ? 0.0 : atan((x * sinc) /
                              (rho * cos_phi1 * cosc - y * sin_phi1 * sinc));

      double lambda = lambda0 + xdist;
      return FromRads(phi, lambda);
    };
}

LatLon::Projection LatLon::Linear(LatLon zerozero, LatLon oneone) {
  const double lattoy = 1.0 / (oneone.lat - zerozero.lat);
  const double lontox = 1.0 / (oneone.lon - zerozero.lon);

  // static constexpr double ytolat = (lat1 - lat0) / (yu1 - yu0);
  // static constexpr double xtolon = (lon1 - lon0) / (xu1 - xu0);

  return [zerozero, lattoy, lontox](LatLon pt) {
      return std::make_pair((pt.lon - zerozero.lon) * lontox,
                            (pt.lat - zerozero.lat) * lattoy);
    };
}
LatLon::InverseProjection LatLon::InverseLinear(LatLon zerozero,
                                                LatLon oneone) {
  const double ytolat = oneone.lat - zerozero.lat;
  const double xtolon = oneone.lon - zerozero.lon;

  return [zerozero, ytolat, xtolon](double x, double y) {
      return LatLon::FromDegs(zerozero.lat + ytolat * y,
                              zerozero.lon + xtolon * x);
    };
}


// Use gnomonic projection. In it, all great circles (from the origin)
// are straight lines. No doubt there is a more direct method, though!
optional<double> LatLon::Angle(LatLon src, LatLon dst) {
  auto Proj = Gnomonic(src);

  // Should always be 0,0 right?
  auto [srcx, srcy] = Proj(src);
  auto [dstx, dsty] = Proj(dst);
  double dx = dstx - srcx;
  double dy = dsty - srcy;
  if (dx == 0.0 && dy == 0.0) return nullopt;

  double angle = atan2(dy, dx);

  // Angle can be negative. fmod twice so that we never get
  // exactly two pi, even if the first one rounds to it.
  double a = fmod(fmod(angle + TWO_PI, TWO_PI), TWO_PI);

  return make_optional(a);
}
