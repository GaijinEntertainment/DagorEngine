// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastTools.h>

#include <generic/dag_staticTab.h>

namespace recastcoll
{
inline bool overlap_range(int amin, int amax, int bmin, int bmax) { return amin < bmin ? bmin < amax : amin < bmax; }

struct VolumeI2D
{
  int maxv;
  int minv;

  VolumeI2D() {} //-V730
  VolumeI2D(int max_in, int min_in) : maxv(max_in), minv(min_in){};
  explicit VolumeI2D(const rcSpan *span) : maxv(span->smax), minv(span->smin){};

  inline void addVolume(const VolumeI2D &other)
  {
    minv = min(minv, other.minv);
    maxv = max(maxv, other.maxv);
  }
  inline void addVolume(const rcSpan *span)
  {
    minv = min(minv, (int)span->smin);
    maxv = max(maxv, (int)span->smax);
  }

  inline bool isOverlapped(const VolumeI2D &other) const { return overlap_range(minv, maxv, other.minv, other.maxv); }
  inline bool isOverlapped(const rcSpan *span) const { return overlap_range(minv, maxv, span->smin, span->smax); }

  inline void cutByVolume(const VolumeI2D &other) // must be overlapped!
  {
    minv = max(other.minv, minv);
    maxv = min(other.maxv, maxv);
  }

  inline int volume() const // len
  {
    return maxv - minv;
  }

  inline void validate()
  {
    if (minv > maxv)
      eastl::swap(minv, maxv);
  }

  inline int nearestPoint(int point) const { return abs(point - minv) < abs(point - maxv) ? minv : maxv; }
};

float check_heightfield_upper_point(const rcHeightfield *m_solid, const Point3 &pt, const float minSpace)
{
  const int w = m_solid->width;
  const int h = m_solid->height;
  const float cs = m_solid->cs;
  const float ch = m_solid->ch;
  const float *orig = m_solid->bmin;
  const int ix = (int)floorf((pt.x - orig[0]) / cs);
  const int iz = (int)floorf((pt.z - orig[2]) / cs);
  float miny = pt.y;

  if (ix < 0 || iz < 0 || ix >= w || iz >= h)
    return -FLT_MAX;

  const rcSpan *s = m_solid->spans[ix + iz * w];
  // const rcSpan *start = s;
  if (!s)
    return -FLT_MAX;

  float prevYMax = FLT_MAX;
  while (s)
  {
    const float symin = orig[1] + s->smin * ch;
    const float symax = orig[1] + s->smax * ch;

    if (symin > miny && symin - minSpace > prevYMax)
      return prevYMax;

    prevYMax = symax;
    s = s->next;
  }

  return max(prevYMax, miny);
}

float get_line_max_height(const rcHeightfield *solid, const Point3 &sp, const Point3 &sq, const float minSpace)
{
  float maxy = -FLT_MAX;

  // +1 is needed here because not all cells are scanned for correct height: in some cases tight walls can be ignored
  int nsamples = (int)ceilf(length(Point3(sp.x - sq.x, 0.f, sp.z - sq.z)) / solid->cs) + 1;

  for (int i = 0; i < nsamples; i++)
  {
    const float u = (float)i / (float)(nsamples - 1);
    Point3 ptSp = lerp(sp, sq, u);
    Point3 ptSq = lerp(sp, sq, 1.f - u);

    ptSp.y = max(maxy, ptSp.y);
    float h = check_heightfield_upper_point(solid, ptSp, minSpace);
    if (h == -FLT_MAX)
      break;
    maxy = max(h, maxy);

    ptSq.y = max(maxy, ptSq.y);
    h = check_heightfield_upper_point(solid, ptSq, minSpace);
    if (h == -FLT_MAX)
      break;
    maxy = max(h, maxy);
  }

  return maxy;
}


void add_heightfield_layers_on_point(const rcHeightfield *solid, const Point3 &pt, const VolumeI2D volume,
  StaticTab<VolumeI2D, 20> &layers)
{
  const int w = solid->width;
  const int h = solid->height;
  const float cs = solid->cs;
  const float *orig = solid->bmin;
  const int ix = (int)floorf((pt.x - orig[0]) / cs);
  const int iz = (int)floorf((pt.z - orig[2]) / cs);

  if (ix < 0 || iz < 0 || ix >= w || iz >= h)
    return;

  const rcSpan *s = solid->spans[ix + iz * w];
  if (!s)
    return;

  while (s)
  {
    if (volume.isOverlapped(s))
    {
      bool isFound = false;
      for (auto &layer : layers)
      {
        if (layer.isOverlapped(s))
        {
          isFound = true;
          layer.addVolume(s);
          break;
        }
      }
      if (!isFound)
        layers.push_back(VolumeI2D(s));
    }
    s = s->next;
  }
}

// 1.f - full blocked
float get_line_transparency(const rcHeightfield *solid, const Point3 &sp, const Point3 &sq, const float maxHeight,
  const float minHeight)
{
  const float cs = solid->cs;
  const float ch = solid->ch;
  const float *orig = solid->bmin;

  int nsamples = (int)ceilf(length(Point3(sp.x - sq.x, 0.f, sp.z - sq.z)) / cs);
  VolumeI2D totalVolume((int)floorf((maxHeight - orig[1]) / ch), (int)floorf((minHeight - orig[1]) / ch));
  if (totalVolume.maxv <= totalVolume.minv)
    return 0.f;

  StaticTab<VolumeI2D, 20> layers;

  for (int i = 0; i < nsamples; i++)
  {
    const float u = (float)i / (float)(nsamples - 1);
    add_heightfield_layers_on_point(solid, lerp(sp, sq, u), totalVolume, layers);
  }

  for (int i = 0; i < layers.size(); i++) // merge overlapped layers
  {
    for (int j = i + 1; j < layers.size(); j++)
    {
      if (!layers[i].isOverlapped(layers[j]))
        continue;
      layers[i].addVolume(layers[j]);

      if (j != (int)layers.size() - 1)
        eastl::swap(layers[j], layers[(int)layers.size() - 1]);
      layers.pop_back();
      --j;
    }
  }

  int blockedVolume = 0;
  for (int i = 0; i < layers.size(); i++)
  {
    layers[i].cutByVolume(totalVolume);
    blockedVolume += layers[i].volume();
  }
  return float(blockedVolume) / float(totalVolume.volume());
}

bool is_point_blocked(const rcHeightfield *m_solid, const IPoint3 &point)
{
  const rcSpan *s = m_solid->spans[point.x + point.z * m_solid->width];
  while (s)
  {
    if (point.y >= s->smin && point.y <= s->smax)
      return true;
    s = s->next;
  }
  return false;
}

bool is_volume_overlapped(const rcHeightfield *m_solid, IPoint3 &point_in_out, const VolumeI2D &vol)
{
  const rcSpan *s = m_solid->spans[point_in_out.x + point_in_out.z * m_solid->width];
  while (s)
  {
    if (vol.isOverlapped(s))
    {
      VolumeI2D span(s);
      span.cutByVolume(vol);
      point_in_out.y = span.nearestPoint(point_in_out.y);
      return true;
    }
    s = s->next;
  }
  return false;
}

bool traceray(const rcHeightfield *m_solid, const Point3 &start_in, Point3 &end_in_out)
{
  int nsamples = (int)ceilf(length(Point3(start_in.x - end_in_out.x, 0.f, start_in.z - end_in_out.z)) / m_solid->cs);

  if (nsamples < 2)
  {
    IPoint3 start = convert_point(m_solid, start_in);
    IPoint3 end = convert_point(m_solid, end_in_out);
    VolumeI2D vol(start.y, end.y);
    vol.validate();
    if (is_volume_overlapped(m_solid, start, vol))
    {
      end_in_out = convert_point(m_solid, start);
      return true;
    }
    return false;
  }

  int hDir = (int)ceilf(((end_in_out.y - start_in.y) * safeinv(length(start_in - end_in_out))) / m_solid->ch);
  for (int i = 0; i < nsamples; i++)
  {
    const float u = (float)i / (float)(nsamples - 1);
    IPoint3 pt = convert_point(m_solid, lerp(start_in, end_in_out, u));

    if (hDir != 0)
    {
      VolumeI2D vol(pt.y, pt.y + hDir);
      vol.validate();
      if (is_volume_overlapped(m_solid, pt, vol))
      {
        end_in_out = convert_point(m_solid, pt);
        return true;
      }
    }
    else if (is_point_blocked(m_solid, pt))
    {
      end_in_out = convert_point(m_solid, pt);
      return true;
    }
  }

  return false;
}

// can be more optimized
float build_box_on_sphere_cast(const rcHeightfield *m_solid, const Point3 &pt, float maxSize, IPoint2 step, BBox3 &box)
{
  box += pt;
  Point3 testPt = pt + Point3(0.f, 0.2f, 0.f);
  int totalCount = 0;
  int findCollisionCount = 0;
  for (int x = 0; x < 360; x += step.x)
  {
    for (int y = 10; y < 70; y += step.y)
    {
      ++totalCount;
      float sinX, cosX;
      float sinY, cosY;
      sincos(DegToRad(x), sinX, cosX);
      sincos(DegToRad(y), sinY, cosY);

      Point3 dir;
      dir.x = cosX * cosY;
      dir.y = sinY;
      dir.z = sinX * cosY;

      Point3 end = testPt + dir * maxSize;
      if (traceray(m_solid, testPt, end))
        ++findCollisionCount;
      box += end;
    }
  }

  return float(findCollisionCount) / float(totalCount);
}

bool get_compact_heightfield_height(const rcCompactHeightfield *chf, const Point3 &pt, const float range, const float hrange,
  float &height)
{
  const int ix0 = clamp((int)floorf((pt.x - range - chf->bmin[0]) / chf->cs), 0, chf->width - 1);
  const int iz0 = clamp((int)floorf((pt.z - range - chf->bmin[2]) / chf->cs), 0, chf->height - 1);
  const int ix1 = clamp((int)floorf((pt.x + range - chf->bmin[0]) / chf->cs), 0, chf->width - 1);
  const int iz1 = clamp((int)floorf((pt.z + range - chf->bmin[2]) / chf->cs), 0, chf->height - 1);

  float bestDist = FLT_MAX;
  float bestHeight = FLT_MAX;
  bool found = false;

  for (int z = iz0; z <= iz1; ++z)
  {
    for (int x = ix0; x <= ix1; ++x)
    {
      const rcCompactCell &c = chf->cells[x + z * chf->width];
      for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
      {
        const rcCompactSpan &s = chf->spans[i];
        if (chf->areas[i] == RC_NULL_AREA)
          continue;
        const float y = chf->bmin[1] + s.y * chf->ch;
        const float dist = abs(y - pt.y);
        if (dist < hrange && dist < bestDist)
        {
          bestDist = dist;
          bestHeight = y;
          found = true;
        }
      }
    }
  }

  height = found ? bestHeight : pt.y;
  return found;
}


bool is_line_walkable(const rcCompactHeightfield *chf, const Point3 &sp, const Point3 &sq, const Point2 precision)
{
  float cs = chf->cs;
  int nsamples = (int)ceilf(length(Point3(sp.x - sq.x, 0.f, sp.z - sq.z)) / cs);

  for (int i = 0; i < nsamples; i++)
  {
    const float u = (float)i / (float)max(nsamples - 1, 1);
    Point3 pt = lerp(sp, sq, u);

    float height = 0.f;
    if (!get_compact_heightfield_height(chf, pt, precision.x, precision.y, height))
      return false;
    if (abs(pt.y - height) > precision.y)
      return false;
  }

  return true;
}
} // namespace recastcoll
