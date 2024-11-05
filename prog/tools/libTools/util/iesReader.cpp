// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/iesReader.h>

#undef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES 1

#include <math/dag_mathUtils.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <fstream>
#include <regex>
#include <thread>

#include <libTools/util/iLogWriter.h>

static constexpr uint32_t IES_MSAA = 4;

struct SortingStructure
{
  IesReader::Real value;
  uint32_t index;
};

IesReader::IesReader(IesParams ies_params) : params(eastl::move(ies_params)) {}

static IesReader::Texcoords get_msaa_texcoord(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t subX, uint32_t subY)
{
  IesReader::Real pixelX = IesReader::Real(x) + (IesReader::Real(subX) + 0.5) / IES_MSAA;
  IesReader::Real pixelY = IesReader::Real(y) + (IesReader::Real(subY) + 0.5) / IES_MSAA;
  return IesReader::Texcoords{pixelX / width, pixelY / height};
}

static void get_dir(const IesReader::SphericalCoordinates &coords, IesReader::Real &dx, IesReader::Real &dy, IesReader::Real &dz)
{
  dz = cos(coords.theta);
  dx = sin(coords.theta) * cos(coords.phi);
  dy = sin(coords.theta) * sin(coords.phi);
}

static IesReader::Real get_angular_distance(const IesReader::SphericalCoordinates &a, const IesReader::SphericalCoordinates &b)
{
  IesReader::Real ax, ay, az;
  IesReader::Real bx, by, bz;
  get_dir(a, ax, ay, az);
  get_dir(b, bx, by, bz);
  IesReader::Real cosAngle = ax * bx + ay * by + az * bz; // dot
  return norm_s_ang(acos(cosAngle));
}

static IesReader::Texcoords inv_map_octahedral(const IesReader::SphericalCoordinates &coords)
{
  IesReader::Real dx, dy, dz;
  get_dir(coords, dx, dy, dz);
  IesReader::Real len = abs(dx) + abs(dy) + abs(dz);
  dx /= len;
  dy /= len;
  dz /= len;
  IesReader::Texcoords ret = dz < 0.0 ? IesReader::Texcoords{(1.0 - abs(dy)) * (dx < 0 ? -1 : 1), (1.0 - abs(dx)) * (dy < 0 ? -1 : 1)}
                                      : IesReader::Texcoords{dx, dy};
  ret.x = ret.x * 0.5 + 0.5;
  ret.y = ret.y * 0.5 + 0.5;
  return ret;
}

static IesReader::SphericalCoordinates map_spherical(const IesReader::Texcoords &tc, IesReader::Real zoom)
{
  return {tc.y * M_PI / zoom, tc.x * M_PI * 2.0};
}

IesReader::SphericalCoordinates get_spherical_coords(IesReader::Real dx, IesReader::Real dy, IesReader::Real dz)
{
  IesReader::Real len = sqrt(dx * dx + dy * dy + dz * dz);
  dx /= len;
  dy /= len;
  dz /= len;
  return {norm_ang(acos(dz)), norm_ang(atan2(dy, dx))};
}

IesReader::SphericalCoordinates map_octahedral(IesReader::Texcoords tc, IesReader::Real zoom, bool rotate)
{
  tc.x = tc.x * 2 - 1;
  tc.y = tc.y * 2 - 1;
  tc.x /= zoom;
  tc.y /= zoom;
  if (rotate)
  {
    IesReader::Real tempX = tc.x;
    tc.x = (tc.x - tc.y) / 2;
    tc.y = (tempX + tc.y) / 2;
  }
  IesReader::Real dx = tc.x;
  IesReader::Real dy = tc.y;
  static_assert(eastl::is_same_v<IesReader::Real, decltype(abs(tc.x))>, "Wrong abs function is used");
  IesReader::Real dz = 1.0f - (abs(tc.x) + abs(tc.y));
  if (abs(tc.x) + abs(tc.y) > 1)
  {
    IesReader::Real absX = abs(dx);
    dx = -(abs(dy) - 1) * sign(dx);
    dy = -(absX - 1) * sign(dy);
  }
  return get_spherical_coords(dx, dy, dz);
}

static IesReader::Real get_area(const IesReader::TransformParams &transform)
{
  IesReader::Real ret = 0;
  if (transform.rotated)
  {
    IesReader::Real len = transform.zoom > 0 ? 1.0 / transform.zoom : 0;
    IesReader::Real totalArea = len * len * 2.0;
    if (len > 1)
    {
      IesReader::Real areaOutside = (len - 1) * (len - 1) * 4;
      totalArea -= areaOutside;
    }
    ret = totalArea;
  }
  else
  {
    ret = transform.zoom > 0 ? 1.0 / transform.zoom : 0;
    ret = ret * ret * 4;
  }
  return ret;
}

static IesReader::SphericalCoordinates get_coords_with_offset(const IesReader::SphericalCoordinates &coords,
  const IesReader::SphericalCoordinates &offset)
{
  IesReader::Real ox, oy, oz;
  get_dir(coords, ox, oy, oz);
  Point3 zAxis(ox, oy, oz);
  Point3 yAxis = abs(oy) < 0.9 ? Point3(0, 1, 0) : Point3(0, 0, 1);
  Point3 xAxis = normalize(cross(yAxis, zAxis));
  yAxis = cross(zAxis, xAxis);
  IesReader::Real radius = tan(offset.theta);
  Point3 p = (yAxis * cos(offset.phi) + xAxis * sin(offset.phi)) * radius;
  return get_spherical_coords(p.x + ox, p.y + oy, p.y + oz);
}

static IesReader::OctahedralBounds get_max_distances(IesReader::Real maxTheta, IesReader::Real phi0, IesReader::Real phi1,
  IesReader::Real phi2)
{
  IesReader::OctahedralBounds ret;
  // abs(x) and abs(y) are both monotonically increasing as theta increases -> take max value
  // of theta
  // abs(x) and abs(y) are both monotonic as phi changes,
  // except for the phi = i * pi/2 (i:[0..3]) values
  // abs(x) + abs(y) is monotonic as phi changes,
  // except for the phi = i * pi/4 (i:[0..7]) values
  IesReader::Texcoords coords0 = inv_map_octahedral(IesReader::SphericalCoordinates{maxTheta, phi0});
  IesReader::Texcoords coords1 = inv_map_octahedral(IesReader::SphericalCoordinates{maxTheta, phi1});
  IesReader::Texcoords coords2 = inv_map_octahedral(IesReader::SphericalCoordinates{maxTheta, phi2});
  coords0.x = coords0.x * 2 - 1;
  coords0.y = coords0.y * 2 - 1;
  coords1.x = coords1.x * 2 - 1;
  coords1.y = coords1.y * 2 - 1;
  coords2.x = coords2.x * 2 - 1;
  coords2.y = coords2.y * 2 - 1;
  ret.norm1 = eastl::max({abs(coords0.x), abs(coords0.y), abs(coords1.x), abs(coords1.x), abs(coords2.x), abs(coords2.x)});
  ret.rectilinear = eastl::max({abs(coords0.x) + abs(coords0.y), abs(coords1.x) + abs(coords1.y), abs(coords2.x) + abs(coords2.y)});
  for (uint32_t i = 0; i < 8; ++i)
  {
    IesReader::Real phi = i * M_PI / 4;
    // Two intervals need to be checked, because phi0 can be phi2, in case of only two sample points
    bool firstInterval = (phi0 <= phi1 && phi0 <= phi && phi <= phi1) || (phi1 <= phi0 && (phi0 <= phi || phi <= phi1));
    bool secondInterval = (phi1 <= phi2 && phi1 <= phi && phi <= phi2) || (phi2 <= phi1 && (phi1 <= phi || phi <= phi2));
    if (firstInterval || secondInterval || (phi0 == phi1 && phi1 == phi2))
    {
      IesReader::Texcoords corners = inv_map_octahedral(IesReader::SphericalCoordinates{maxTheta, phi});
      corners.x = corners.x * 2 - 1;
      corners.y = corners.y * 2 - 1;
      // corner is inside one of [phi0, phi2]
      // phi0 == phi1 == phi2 -> there is only one sample point -> [phi0, phi2] is full range
      ret.rectilinear = eastl::max(ret.rectilinear, abs(corners.x) + abs(corners.y));
      ret.norm1 = eastl::max({ret.norm1, abs(corners.x), abs(corners.y)});
    }
  }
  return ret;
}

bool IesReader::read(const char *filename, ILogWriter &log)
{
  std::ifstream in(filename);
  if (!in)
  {
    log.addMessage(ILogWriter::ERROR, "File could not be opened: %s", filename);
    return false;
  }
  in.sync_with_stdio(false);
  std::string line;
  std::regex version{"IESNA:(.*)"};
  std::regex entry{"\\[(.*)\\](.*)"};
  std::regex tilt{"TILT( )?=( )?([^ ]*)"};
  std::smatch m;
  std::getline(in, line);
  if (!std::regex_match(line, m, version))
    log.addMessage(ILogWriter::WARNING, "Ies file does not begin with 'IESNA' declaration (%s)", filename);
  while (getline(in, line) && std::regex_match(line, m, entry))
  {
    // handle entry here, if needed
  }
  if (!std::regex_match(line, m, tilt))
  {
    log.addMessage(ILogWriter::ERROR, "Missing 'TILT' entry (or wrong format) in file: %s", filename);
    return false;
  }
  // handle tilt here, if needed
  uint32_t numHorizontalAngles = 0;
  uint32_t numVerticalAngles = 0;
  in >> lamps;
  in >> lumensPerLamp;
  in >> multiplier;
  in >> numVerticalAngles;
  in >> numHorizontalAngles;
  in >> photometricType;
  in >> unitsType;
  in >> dirX;
  in >> dirY;
  in >> dirZ;
  in >> ballastFactor;
  in >> futureUse;
  in >> inputWatts;
  maxTheta = 0;
  octahedralBounds.norm1 = 0;
  octahedralBounds.rectilinear = 0;
  eastl::vector<SortingStructure> vertical(numVerticalAngles);
  eastl::vector<SortingStructure> horizontal(numHorizontalAngles);
  eastl::vector<uint32_t> vMapping(numVerticalAngles);
  eastl::vector<uint32_t> hMapping(numHorizontalAngles);
  verticalAngles.resize(numVerticalAngles);
  horizontalAngles.resize(numHorizontalAngles);
  for (uint32_t v = 0; v < numVerticalAngles; ++v)
  {
    in >> vertical[v].value;
    vertical[v].value = clamp(vertical[v].value * DEG_TO_RAD, 0.0, M_PI);
    vertical[v].index = v;
  }
  for (uint32_t h = 0; h < numHorizontalAngles; ++h)
  {
    in >> horizontal[h].value;
    horizontal[h].value = norm_ang(horizontal[h].value * DEG_TO_RAD);
    horizontal[h].index = h;
  }
  eastl::sort(eastl::begin(vertical), eastl::end(vertical),
    [](const SortingStructure &lhs, const SortingStructure &rhs) { return lhs.value < rhs.value; });
  eastl::sort(eastl::begin(horizontal), eastl::end(horizontal),
    [](const SortingStructure &lhs, const SortingStructure &rhs) { return lhs.value < rhs.value; });
  for (uint32_t h = 0; h < numHorizontalAngles; ++h)
  {
    horizontalAngles[h] = horizontal[h].value;
    hMapping[horizontal[h].index] = h;
  }
  for (uint32_t v = 0; v < numVerticalAngles; ++v)
  {
    verticalAngles[v] = vertical[v].value;
    vMapping[vertical[v].index] = v;
  }
  candelaValues.resize(numHorizontalAngles);
  maxIntensity = 0;
  for (uint32_t h = 0; h < numHorizontalAngles; ++h)
  {
    candelaValues[h].resize(numVerticalAngles);
    for (uint32_t v = 0; v < numVerticalAngles; ++v)
    {
      const uint32_t hInd = hMapping[h];
      const uint32_t vInd = vMapping[v];
      in >> candelaValues[hInd][vInd];
      if (candelaValues[hInd][vInd] > 0)
      {
        // The current intensity values can affect the angle range:
        // from (verticalAngles[vInd+1], horizontalAngles[hInd-1])
        // to (verticalAngles[vInd+1], horizontalAngles[hInd+1])
        const Real theta = clamp(vInd + 1 < verticalAngles.size() ? verticalAngles[vInd + 1] : M_PI, params.thetaMin, params.thetaMax);
        OctahedralBounds bounds = get_max_distances(theta, horizontalAngles[hInd], horizontalAngles[previousHorizontalIndex(hInd)],
          horizontalAngles[nextHorizontalIndex(hInd)]);
        octahedralBounds.rectilinear = eastl::max(octahedralBounds.rectilinear, bounds.rectilinear);
        octahedralBounds.norm1 = eastl::max(octahedralBounds.norm1, bounds.norm1);
        maxTheta = max(maxTheta, theta);
      }
      maxIntensity = max(maxIntensity, candelaValues[hInd][vInd]);
    }
  }
  if (in.eof())
  {
    log.addMessage(ILogWriter::ERROR, "File format error in %s. EOF was reached before expected", filename);
    return false;
  }
  return true;
}

static IesReader::PixelType real_to_pixel(IesReader::Real value)
{
  return eastl::max(IesReader::Real(0.0), eastl::min(value, IesReader::Real(1.0)));
}

IesReader::TransformParams IesReader::getSphericalOptimalTransform(uint32_t /*width*/, uint32_t height) const
{
  Real blurTheta = (ceil(params.blurRadius / M_PI * height) / height) * 2;
  TransformParams ret;
  ret.zoom = maxTheta > 0 ? M_PI / eastl::min(maxTheta + blurTheta, M_PI) : 1;
  return ret;
}

IesReader::ImageData IesReader::generateSpherical(uint32_t width, uint32_t height, const TransformParams &transform) const
{
  uint32_t baseVInd = uint32_t(verticalAngles.size());
  uint32_t baseHInd = uint32_t(horizontalAngles.size());
  ImageData ret;
  ret.height = height;
  ret.width = width;
  ret.transform = transform;
  ret.data = eastl::vector<PixelType>(width * height);
  auto get_tc = [width, height](uint32_t w, uint32_t h, uint32_t subW, uint32_t subH) {
    Texcoords tc = get_msaa_texcoord(width, height, w, h, subW, subH);
    Real integerPart;
    // match the sides of the texture
    tc.x = modf(tc.x + tc.x / width, &integerPart);
    return Texcoords{tc.x, tc.y};
  };
  for (uint32_t h = 0; h < height; ++h)
  {
    for (uint32_t w = 0; w < width; ++w)
    {
      uint32_t index = h * width + w;
      Real sum = 0;

      for (uint32_t subH = 0; subH < IES_MSAA; ++subH)
      {
        for (uint32_t subW = 0; subW < IES_MSAA; ++subW)
        {
          SphericalCoordinates currentCoordinates = map_spherical(get_tc(w, h, subW, subH), ret.transform.zoom);
          Real fadeout = params.edgeFadeout;
          if (fadeout < 0)
          {
            SphericalCoordinates diffCoordinates =
              map_spherical(get_tc(w > width / 2 ? w - 1 : w + 1, h > height / 2 ? h - 1 : h + 1, subW, subH), ret.transform.zoom);
            fadeout = get_angular_distance(currentCoordinates, diffCoordinates);
          }
          Real s = sample_filtered(currentCoordinates, fadeout, baseHInd, baseVInd);
          sum += s;
        }
      }
      ret.data[index] = real_to_pixel((sum / (IES_MSAA * IES_MSAA)) / maxIntensity);
    }
  }
  return ret;
}

IesReader::TransformParams IesReader::getOctahedralOptimalTransform(uint32_t width, uint32_t height, const bool *forced_rotation) const
{
  Real xyBlur = eastl::max(width, height) / (2.0 * M_PI) * params.blurRadius;
  xyBlur = 2.0 * ceil(xyBlur) / eastl::max(width, height);
  Real distBlur = xyBlur;

  Real blurredDist = eastl::min(octahedralBounds.rectilinear + distBlur, M_SQRT2);
  Real blurredXY = eastl::min(octahedralBounds.norm1 + xyBlur, 1.0);
  TransformParams ret;
  if (abs(blurredDist) >= 0.0001)
  {
    // This is the optimal threshold for rotating the texture
    ret.rotated = forced_rotation == nullptr ? blurredDist * blurredDist < 2 : *forced_rotation;
    ret.zoom = 1.0 / (ret.rotated ? blurredDist : blurredXY);
  }
  return ret;
}

bool IesReader::isSphericalTransformValid(uint32_t width, uint32_t height, const TransformParams &transform) const
{
  TransformParams optimum = getSphericalOptimalTransform(width, height);
  return transform.zoom <= optimum.zoom && !transform.rotated;
}

bool IesReader::isOctahedralTransformValid(uint32_t width, uint32_t height, const TransformParams &transform) const
{
  TransformParams optimum = getOctahedralOptimalTransform(width, height, &transform.rotated);
  return transform.zoom <= optimum.zoom;
}

IesReader::Real IesReader::getSphericalRelativeWaste(uint32_t width, uint32_t height, const TransformParams &transform) const
{
  TransformParams optimum = getSphericalOptimalTransform(width, height);
  G_ASSERT(!transform.rotated && !optimum.rotated);
  if (optimum.zoom <= transform.zoom)
    return 0;
  Real textureSpace = transform.zoom > 0 ? 1.0 / transform.zoom : 0;
  Real usedSpace = optimum.zoom > 0 ? 1.0 / optimum.zoom : 0;
  return textureSpace - usedSpace;
}

IesReader::Real IesReader::getOctahedralRelativeWaste(uint32_t width, uint32_t height, const TransformParams &transform) const
{
  TransformParams optimum = getOctahedralOptimalTransform(width, height);
  Real textureSpace = get_area(transform);
  Real usedSpace = get_area(optimum);
  return (textureSpace - usedSpace) / 4;
}

IesReader::ImageData IesReader::generateOctahedral(uint32_t width, uint32_t height, const TransformParams &transform) const
{
  G_ASSERTF(params.blurRadius < M_PI / 4, "Blur radius is too large");
  G_ASSERTF(transform.zoom >= M_SQRT1_2, "Invalid ies texture scaling: %f must be at least %f", transform.zoom, M_SQRT1_2);
  uint32_t baseVInd = uint32_t(verticalAngles.size());
  uint32_t baseHInd = uint32_t(horizontalAngles.size());
  ImageData ret;
  ret.height = height;
  ret.width = width;
  ret.data = eastl::vector<PixelType>(width * height);
  ret.transform = transform;
  auto get_tc = [width, height](uint32_t w, uint32_t h, uint32_t subW, uint32_t subH) {
    return get_msaa_texcoord(width, height, w, h, subW, subH);
  };
  for (uint32_t h = 0; h < height; ++h)
  {
    for (uint32_t w = 0; w < width; ++w)
    {
      uint32_t index = h * width + w;
      Real sum = 0;
      // Pixes offset is gradually changed to get the touching sides
      // of the texture close to each other the texture is folded in
      // a unique way, so built in interpolation of the texture
      // cannot be used. This avoids haveing a visible tear in the
      // texture at the fold-lines.
      for (uint32_t subH = 0; subH < IES_MSAA; ++subH)
      {
        for (uint32_t subW = 0; subW < IES_MSAA; ++subW)
        {
          SphericalCoordinates currentCoordinates =
            map_octahedral(get_tc(w, h, subW, subH), ret.transform.zoom, ret.transform.rotated);
          Real fadeout = params.edgeFadeout;
          if (fadeout < 0)
          {
            uint32_t nextCoordX = w > width / 2 ? w - 1 : w + 1;
            uint32_t nextCoordY = h > height / 2 ? h - 1 : h + 1;
            SphericalCoordinates diffCoordinates =
              map_octahedral(get_tc(nextCoordX, nextCoordY, subW, subH), ret.transform.zoom, ret.transform.rotated);
            fadeout = get_angular_distance(currentCoordinates, diffCoordinates);
          }
          Real s = sample_filtered(currentCoordinates, fadeout, baseHInd, baseVInd);
          sum += s;
        }
      }
      ret.data[index] = real_to_pixel(sum / (IES_MSAA * IES_MSAA) / maxIntensity);
    }
  }
  return ret;
}

static IesReader::Real get_angular_distance_from_range(IesReader::Real angle, IesReader::Real min_angle, IesReader::Real max_angle)
{
  if (max_angle < min_angle)
    max_angle += M_PI * 2;
  IesReader::Real closestPeriodToMin = min_angle + angle_diff(min_angle, angle);
  IesReader::Real closestPeriodToMax = max_angle + angle_diff(max_angle, angle);
  return min(max(min_angle - closestPeriodToMin, closestPeriodToMin - max_angle),
    max(min_angle - closestPeriodToMax, closestPeriodToMax - max_angle));
}

static bool is_angle_between(IesReader::Real angle, IesReader::Real min, IesReader::Real max)
{
  return get_angular_distance_from_range(angle, min, max) <= 0;
}

IesReader::Real IesReader::sample_raw(SphericalCoordinates coords, uint32_t &h, uint32_t &v) const
{
  // h and v are expected to hold the specified coordinate
  // if not, the correct h and v values are one of the neighbouring cells
  // h and v are set, so the next sample can be done efficiently the next time
  coords.phi = norm_ang(coords.phi);
  while (v > 0 && verticalAngles[v] > coords.theta)
    v--;
  while (v < verticalAngles.size() && verticalAngles[v + 1] < coords.theta)
    v++;
  if (horizontalAngles.size() > 1)
  {
    // pick which direction the correct cell is closer.
    // Both directions would work, it's just for performance
    uint32_t step = angle_diff(horizontalAngles[h], coords.phi) >= 0 ? 1                            // forwards
                                                                     : horizontalAngles.size() - 1; // backwards
    uint32_t startH = h;
    for (uint32_t i = 0; i < horizontalAngles.size(); ++i)
    {
      h = offsetHorizontalIndex(startH, step * i);
      if (is_angle_between(coords.phi, horizontalAngles[h], horizontalAngles[nextHorizontalIndex(h)]))
        break;
    }
    G_ASSERT(is_angle_between(coords.phi, horizontalAngles[h], horizontalAngles[nextHorizontalIndex(h)]));
  }
  Real hd = norm_ang(horizontalAngles[nextHorizontalIndex(h)] - horizontalAngles[h]);
  Real vd = v + 1 < verticalAngles.size() ? verticalAngles[v + 1] - verticalAngles[v] : static_cast<Real>(M_PI) - verticalAngles[v];
  Real x = hd > 0 ? norm_ang(coords.phi - horizontalAngles[h]) / hd : 0.5; // take avg
  Real y = vd > 0 ? (coords.theta - verticalAngles[v]) / vd : 0.5;

  uint32_t v2 = min<uint32_t>(v + 1, verticalAngles.size() - 1);
  uint32_t h2 = nextHorizontalIndex(h);

  return lerp(lerp(candelaValues[h][v], candelaValues[h][v2], y), lerp(candelaValues[h2][v], candelaValues[h2][v2], y), x);
}

IesReader::Real IesReader::sample_filtered(SphericalCoordinates coords, Real fadeout, uint32_t &h, uint32_t &v) const
{
  coords.phi = norm_ang(coords.phi);
  if (coords.theta < 0 || coords.theta > M_PI)
    return 0;
  if (verticalAngles.size() * horizontalAngles.size() == 0)
    return 0;
  Real signedDist = max(get_angular_distance_from_range(coords.phi, params.phiMin, params.phiMax),
    max(params.thetaMin - coords.theta, coords.theta - params.thetaMax));
  if (signedDist > fadeout)
    return 0;
  Real fadeoutMul = signedDist > 0 ? ssmooth(signedDist / fadeout, 1.0, 0.0) : 1;
  if (h >= horizontalAngles.size() || v >= verticalAngles.size())
  {
    h = 0;
    if (coords.phi >= horizontalAngles.front())
    {
      uint32_t b = static_cast<uint32_t>(horizontalAngles.size());
      while (h + 1 < b)
      {
        uint32_t m = (h + b) / 2;
        if (horizontalAngles[m] <= coords.phi)
          h = m;
        else
          b = m;
      }
    }
    else
      h = static_cast<uint32_t>(horizontalAngles.size() - 1);
    v = 0;
    uint32_t b = static_cast<uint32_t>(verticalAngles.size());
    while (v + 1 < b)
    {
      uint32_t m = (v + b) / 2;
      if (verticalAngles[m] < coords.theta)
        v = m;
      else
        b = m;
    }
  }

  if (params.blurRadius <= 0)
  {
    return sample_raw(coords, h, v) * fadeoutMul;
  }
  else
  {
    Real sum = 0;
    Real weight = 0;
    uint32_t circularSamples = ceil(sqrt(params.blurRadius * RAD_TO_DEG)) * 4;
    uint32_t radiusSamples = ceil(params.blurRadius * RAD_TO_DEG) * 2;
    Real sigma = params.blurRadius / 2.5;
    Real phiDiff = 1.0 / circularSamples * M_PI * 2;
    for (uint32_t j = 0; j < radiusSamples; ++j)
    {
      Real theta0 = Real(j) / radiusSamples * params.blurRadius;
      Real theta1 = Real(j + 1) / radiusSamples * params.blurRadius;
      Real theta = (theta0 + theta1) / 2;
      Real area = phiDiff * (cos(theta0) - cos(theta1));
      // gauss filter in the function of theta (theta = radial deviation from the sampled point)
      Real filter = exp(-(theta / sigma) * (theta / sigma) / 2) / (sqrt(2.0 * M_PI) / sigma);
      for (uint32_t i = 0; i < circularSamples; ++i)
      {
        // '+ 0.5 * j' will cause a staggered pattern of samples
        Real phi = (Real(i) + 0.5 * j) / circularSamples * M_PI * 2;
        Real w = area * filter;
        sum += sample_raw(get_coords_with_offset(coords, {theta, phi}), h, v) * w;
        weight += w;
      }
    }
    return safediv(sum, weight) * fadeoutMul;
  }
}
