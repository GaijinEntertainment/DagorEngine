#include <math/dag_math2d.h>

bool VECTORCALL isect_line_box(Point2 p0, Point2 dir, const BBox2 &box, real &min_t, real &max_t)
{
  real t0, t1, t;

  if (dir.x > 0)
  {
    if (dir.y == 0)
      if (p0.y < box[0].y || p0.y > box[1].y)
        return false;

    t0 = (box[0].x - p0.x) / dir.x;
    t1 = (box[1].x - p0.x) / dir.x;
  }
  else if (dir.x < 0)
  {
    if (dir.y == 0)
      if (p0.y < box[0].y || p0.y > box[1].y)
        return false;

    t0 = (box[1].x - p0.x) / dir.x;
    t1 = (box[0].x - p0.x) / dir.x;
  }
  else
  {
    if (dir.y == 0)
    {
      if (!(box & p0))
        return false;
      min_t = 0;
      max_t = 0;
      return true;
    }
    else
    {
      if (p0.x < box[0].x || p0.x > box[1].x)
        return false;
      t0 = MIN_REAL;
      t1 = MAX_REAL;
    }
  }

  if (dir.y > 0)
  {
    t = (box[0].y - p0.y) / dir.y;
    if (t > t0)
      t0 = t;
    t = (box[1].y - p0.y) / dir.y;
    if (t < t1)
      t1 = t;
  }
  else if (dir.y < 0)
  {
    t = (box[1].y - p0.y) / dir.y;
    if (t > t0)
      t0 = t;
    t = (box[0].y - p0.y) / dir.y;
    if (t < t1)
      t1 = t;
  }

  if (t0 > t1)
    return false;

  min_t = t0;
  max_t = t1;
  return true;
}

int VECTORCALL isect_line_segment_box(Point2 p1, Point2 p2, const BBox2 &box)
{
  Point2 lmin = p1, lmax = p2;

  if (lmin.x > lmax.x)
  {
    real a = lmin.x;
    lmin.x = lmax.x;
    lmax.x = a;
  }
  if (lmin.y > lmax.y)
  {
    real a = lmin.y;
    lmin.y = lmax.y;
    lmax.y = a;
  }

  if (lmin.x > box[1].x || lmax.x < box[0].x || lmin.y > box[1].y || lmax.y < box[0].y)
    return 0;

  real t0, t1;
  if (!isect_line_box(p1, p2 - p1, box, t0, t1))
    return 0;

  if (t0 > 1 || t1 < 0)
    return 0;
  if (t0 >= 0 && t1 <= 1)
    return 2;
  return 1;
}

//==============================================================================
int VECTORCALL get_nearest_point_index(Point2 p, Point2 *pts, int num)
{
  float bestLenSq = FLT_MAX;
  int nearestIndex = -1;
  for (int i = 0; i < num; i++)
  {
    float lenSq = (p - pts[i]).lengthSq();
    if (lenSq < bestLenSq)
    {
      bestLenSq = lenSq;
      nearestIndex = i;
    }
  }
  return nearestIndex;
}

// Checks lines for intersection
bool VECTORCALL get_lines_intersection(Point2 st1, Point2 en1, Point2 st2, Point2 en2, Point2 &intPt)
{
  Point2 v1 = en1 - st1;
  Point2 v2 = en2 - st2;
  float det = v1.x * v2.y - v1.y * v2.x;

  if (fabsf(det) < FLT_EPSILON)
    return false;

  float t = (v2.x * (st1.y - st2.y) - v2.y * (st1.x - st2.x)) / det;
  intPt = st1 + v1 * t;

  return true;
}

// Checks lines for intersection


// Checks segments for intersection
bool VECTORCALL get_lines_intersection(Point2 start1, Point2 end1, Point2 start2, Point2 end2, Point2 *out_intersection = NULL)
{
  Point2 dir1 = end1 - start1;
  Point2 dir2 = end2 - start2;

  float a1 = -dir1.y;
  float b1 = +dir1.x;
  float d1 = -(a1 * start1.x + b1 * start1.y);

  float a2 = -dir2.y;
  float b2 = +dir2.x;
  float d2 = -(a2 * start2.x + b2 * start2.y);

  float seg1_line2_start = a2 * start1.x + b2 * start1.y + d2;
  float seg1_line2_end = a2 * end1.x + b2 * end1.y + d2;

  float seg2_line1_start = a1 * start2.x + b1 * start2.y + d1;
  float seg2_line1_end = a1 * end2.x + b1 * end2.y + d1;

  if (seg1_line2_start * seg1_line2_end >= 0 || seg2_line1_start * seg2_line1_end >= 0)
    return false;

  float u = seg1_line2_start / (seg1_line2_start - seg1_line2_end);

  if (out_intersection)
    *out_intersection = start1 + u * dir1;

  return true;
}

bool VECTORCALL is_lines_intersect(const Point2 p11, const Point2 p12, const Point2 p21, const Point2 p22)
{
  real x1, x2, x3, x4, y1, y2, y3, y4;

  if (p11.x > p12.x)
  {
    x1 = p12.x;
    x2 = p11.x;
  }
  else
  {
    x1 = p11.x;
    x2 = p12.x;
  }

  if (p21.x > p22.x)
  {
    x3 = p22.x;
    x4 = p21.x;
  }
  else
  {
    x3 = p21.x;
    x4 = p22.x;
  }

  if (p11.y > p12.y)
  {
    y1 = p12.y;
    y2 = p11.y;
  }
  else
  {
    y1 = p11.y;
    y2 = p12.y;
  }

  if (p21.y > p22.y)
  {
    y3 = p22.y;
    y4 = p21.y;
  }
  else
  {
    y3 = p21.y;
    y4 = p22.y;
  }

  if ((x2 >= x3) && (x4 >= x1) && (y2 >= y3) && (y4 >= y1))
  {
    x1 = p12.x - p11.x;
    y1 = p12.y - p11.y;
    x2 = p22.x - p11.x;
    y2 = p22.y - p11.y;
    x3 = p21.x - p11.x;
    y3 = p21.y - p11.y;

    if (((x1 * y2 - x2 * y1) * (x1 * y3 - x3 * y1)) > 0)
      return false;

    x1 = p21.x - p22.x;
    y1 = p21.y - p22.y;
    x2 = p12.x - p22.x;
    y2 = p12.y - p22.y;
    x3 = p11.x - p22.x;
    y3 = p11.y - p22.y;

    if (((x1 * y2 - x2 * y1) * (x1 * y3 - x3 * y1)) > 0)
      return false;

    return true;
  }

  return false;
}

real VECTORCALL distance_point_to_line_segment(Point2 pt, Point2 p1, Point2 p2)
{
  Point2 dp = pt - p1;
  Point2 dir = p2 - p1;
  real len2 = lengthSq(dir);

  if (len2 == 0)
    return length(dp);

  real t = (dp * dir) / len2;

  if (t <= 0)
    return length(dp);
  if (t >= 1)
    return length(pt - p2);

  return rabs(dp * Point2(dir.y, -dir.x)) / sqrtf(len2);
}

//==============================================================================
bool VECTORCALL is_point_in_conv_poly(Point2 p, const Point2 *poly, unsigned count)
{
  if (count < 3)
    return false;

  real fi1 = (p.y - poly[0].y) * (poly[1].x - poly[0].x) - (p.x - poly[0].x) * (poly[1].y - poly[0].y);

  real fi2;
  for (unsigned i = 1; i < count - 1; ++i)
  {
    fi2 = (p.y - poly[i].y) * (poly[i + 1].x - poly[i].x) - (p.x - poly[i].x) * (poly[i + 1].y - poly[i].y);

    if (DIFF_SIGN(fi1, fi2))
      return false;

    fi1 = fi2;
  }

  fi2 = (p.y - poly[count - 1].y) * (poly[0].x - poly[count - 1].x) - (p.x - poly[count - 1].x) * (poly[0].y - poly[count - 1].y);

  return !(DIFF_SIGN(fi1, fi2));
}


//==============================================================================
bool is_point_in_poly(const Point2 &p, const Point2 *poly, int count)
{
  if (count < 3)
    return false;

  unsigned int res = 0;
  Point2 a = poly[count - 1], b;
  for (const Point2 *pb = poly, *pe = poly + count; pb != pe; a = b, ++pb)
  {
    b = *pb;

    if ((p.x < a.x && p.x < b.x) || (p.x >= a.x && p.x >= b.x))
      continue;

    if (b.x > a.x)
    {
      if ((b.y - a.y) * (p.x - a.x) > (p.y - a.y) * (b.x - a.x))
        res++;
    }
    else
    {
      if ((a.y - b.y) * (p.x - b.x) > (p.y - b.y) * (a.x - b.x))
        res++;
    }
  }

  return res & 1;
}

int VECTORCALL inter_circle_rect(Point2 c, real r, const BBox2 &rect)
{
  const BBox2 circRect(c, r * 2);
  int result = inter_rects(circRect, rect);

  if (result != 1)
    return result;
  else
  {
    if (is_point_in_rect(Point2(c.x, c.y - r), rect) || is_point_in_rect(Point2(c.x + r, c.y), rect) ||
        is_point_in_rect(Point2(c.x, c.y + r), rect) || is_point_in_rect(Point2(c.x - r, c.y), rect))
      return 1;

    if (is_point_in_circle(rect[0], c, r) || is_point_in_circle(Point2(rect[1].x, rect[0].y), c, r) ||
        is_point_in_circle(rect[1], c, r) || is_point_in_circle(Point2(rect[0].x, rect[1].y), c, r))
      return 1;
  }

  return 0;
}

bool VECTORCALL isect_box_triangle(const BBox2 &box, Point2 t1, Point2 t2, Point2 t3)
{
  if (is_point_in_triangle(box.leftTop(), t1, t2, t3))
    return true;
  if (is_point_in_triangle(box.leftBottom(), t1, t2, t3))
    return true;
  if (is_point_in_triangle(box.rightTop(), t1, t2, t3))
    return true;
  if (is_point_in_triangle(box.rightBottom(), t1, t2, t3))
    return true;

  if (isect_line_segment_box(t1, t2, box))
    return true;
  if (isect_line_segment_box(t2, t3, box))
    return true;
  if (isect_line_segment_box(t1, t3, box))
    return true;

  return false;
}
