#include <math/dag_math3d.h>
#include <math/dag_noise.h>
#include <math/dag_plane3.h>
#include <debug/dag_debug.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <util/dag_stdint.h>
#include <math/dag_TMatrix4D.h>
#include <math/dag_mathUtils.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>

#if _TARGET_PC && !_TARGET_SIMD_NEON
#if _MSC_VER <= 1310
#include <xmmintrin.h>
#else
#include <intrin.h>
#endif
#endif

static const int QNaNint = 0xFFFFFFFF;
static const int SNaNint = 0xFFBFFFFF;
static const int64_t QNaNint64 = 0xFFFFFFFFFFFFFFFFULL;
static const int64_t SNaNint64 = 0xFFF7FFFFFFFFFFFFULL;

const float realQNaN = bitwise_cast<float, int>(QNaNint);
const float realSNaN = bitwise_cast<float, int>(SNaNint);
const double doubleQNaN = bitwise_cast<double, int64_t>(QNaNint64);
const double doubleSNaN = bitwise_cast<double, int64_t>(SNaNint64);

const Matrix3 Matrix3::IDENT(1.f), Matrix3::ZERO(0.f);
const TMatrix TMatrix::IDENT(1), TMatrix::ZERO(0);
const TMatrix4 TMatrix4::IDENT(1), TMatrix4::ZERO(0);
const BBox3 BBox3::IDENT(Point3(), 1.f);

const IPoint2 IPoint2::ZERO = IPoint2(0, 0);
const IPoint2 IPoint2::ONE = IPoint2(1, 1);
const IPoint3 IPoint3::ZERO = IPoint3(0, 0, 0);
const IPoint3 IPoint3::ONE = IPoint3(1, 1, 1);
const IPoint4 IPoint4::ZERO = IPoint4(0, 0, 0, 0);
const IPoint4 IPoint4::ONE = IPoint4(1, 1, 1, 1);

const Point2 Point2::ZERO = Point2(0, 0);
const Point2 Point2::ONE = Point2(1, 1);
const Point3 Point3::ZERO = Point3(0, 0, 0);
const Point3 Point3::ONE = Point3(1, 1, 1);
const Point4 Point4::ZERO = Point4(0, 0, 0, 0);
const Point4 Point4::ONE = Point4(1, 1, 1, 1);

#if _TARGET_PC_WIN | _TARGET_XBOX
const float __declspec(align(16)) math_float_zero[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined(__GNUC__)
const float math_float_zero[16] __attribute__((aligned(16))) = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

void init_math()
{
  perlin_noise::init_noise(1120272305);
#if _TARGET_SIMD_SSE
  _mm_setcsr((_mm_getcsr() & ~_MM_ROUND_MASK) | _MM_FLUSH_ZERO_MASK | _MM_ROUND_NEAREST | 0x40); // 0x40 - denorms are zero.
  // it is helpful to have 0x40 - denorms are zero - flag, for performance reasons.
  // it should not matter for our math, since we do have _MM_FLUSH_ZERO_MASK (flush-to-zero), but if we load thrash from memory, which
  // we won't use anyway (consider .w in vertex position), it will affect performance! debug_ctx("set DAZ and RN flags");
#endif
}

static inline void calc_screen_box(BBox2 &scbox, Point2 pt[8], BBox3 &b, TMatrix4 &gtm)
{
  scbox.setempty();
  if (b.isempty())
    return;
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
    {
      Point4 a1(b.lim[i].x, b.lim[j].y, b.lim[0].z, 1);
      Point4 a2(b.lim[i].x, b.lim[j].y, b.lim[1].z, 1);
      a1 = a1 * gtm;
      a1.unify();
      a2 = a2 * gtm;
      a2.unify();
      pt[i * 2 + j] = Point2(a1.x, a1.y);
      pt[i * 2 + j + 4] = Point2(a2.x, a2.y);
      scbox += Point2(a1.x, a1.y);
      scbox += Point2(a2.x, a2.y);
    }
}

static inline int p2get_side(Point2 a, Point2 b, Point2 p)
{
  p -= a;
  b = Point2(a.y - b.y, b.x - a.x);
  real c = p * b;
  if (c < 0)
    return -1;
  else
    return 1;
}

bool is_pt_inscreen_box(Point2 &p, BBox3 &b, TMatrix4 &gtm)
{
  BBox2 sc;
  Point2 pt[8];
  calc_screen_box(sc, pt, b, gtm);
  if (!(sc & p))
    return false;
  Point4 bc(b.center().x, b.center().y, b.center().z, 1);
  bc = bc * gtm;
  if (bc.z < 0)
    return false;
  {
    int a = p2get_side(pt[0], pt[0 + 4], p) + p2get_side(pt[1], pt[1 + 4], p);
    if (!a)
    {
      a = p2get_side(pt[0], pt[1], p) + p2get_side(pt[0 + 4], pt[1 + 4], p);
      if (!a)
        return true;
    }
  }

  {
    int a = p2get_side(pt[0], pt[0 + 4], p) + p2get_side(pt[2], pt[2 + 4], p);
    if (!a)
    {
      a = p2get_side(pt[0], pt[2], p) + p2get_side(pt[0 + 4], pt[2 + 4], p);
      if (!a)
        return true;
    }
  }

  {
    int a = p2get_side(pt[2], pt[2 + 4], p) + p2get_side(pt[3], pt[3 + 4], p);
    if (!a)
    {
      a = p2get_side(pt[2], pt[3], p) + p2get_side(pt[2 + 4], pt[3 + 4], p);
      if (!a)
        return true;
    }
  }

  {
    int a = p2get_side(pt[0], pt[2], p) + p2get_side(pt[1], pt[3], p);
    if (!a)
    {
      a = p2get_side(pt[0], pt[1], p) + p2get_side(pt[2], pt[3], p);
      if (!a)
        return true;
    }
  }

  {
    int a = p2get_side(pt[4], pt[6], p) + p2get_side(pt[5], pt[7], p);
    if (!a)
    {
      a = p2get_side(pt[4], pt[5], p) + p2get_side(pt[6], pt[7], p);
      if (!a)
        return true;
    }
  }

  {
    int a = p2get_side(pt[1], pt[1 + 4], p) + p2get_side(pt[3], pt[3 + 4], p);
    if (!a)
    {
      a = p2get_side(pt[1], pt[3], p) + p2get_side(pt[1 + 4], pt[3 + 4], p);
      if (!a)
        return true;
    }
  }

  return false;
}


/*
void RandomRangedReal::load(class DataBlock &blk, float def_val, float def_dev)
{
  val=blk.getReal("val", def_val);
  dev=blk.getReal("dev", def_dev);
}
*/

/*
 * double = det2x2( double a, double b, double c, double d )
 *
 * calculate the determinant of a 2x2 matrix.
 */

static double det2x2(double a, double b, double c, double d)
{
  double ans;
  ans = a * d - b * c;
  return ans;
}


/*
 * double = det3x3(  a1, a2, a3, b1, b2, b3, c1, c2, c3 )
 *
 * calculate the determinant of a 3x3 matrix
 * in the form
 *
 *     | a1,  b1,  c1 |
 *     | a2,  b2,  c2 |
 *     | a3,  b3,  c3 |
 */

static double det3x3(double a1, double a2, double a3, double b1, double b2, double b3, double c1, double c2, double c3)
{
  double ans;

  ans = a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
  return ans;
}


/*
 * double = det4x4( matrix )
 *
 * calculate the determinant of a 4x4 matrix.
 */
template <class T>
inline double det4x4(const T &m)
{
  double ans;
  double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

  /* assign to individual variable names to aid selecting */
  /*  correct elements */

  a1 = m.m[0][0];
  b1 = m.m[0][1];
  c1 = m.m[0][2];
  d1 = m.m[0][3];

  a2 = m.m[1][0];
  b2 = m.m[1][1];
  c2 = m.m[1][2];
  d2 = m.m[1][3];

  a3 = m.m[2][0];
  b3 = m.m[2][1];
  c3 = m.m[2][2];
  d3 = m.m[2][3];

  a4 = m.m[3][0];
  b4 = m.m[3][1];
  c4 = m.m[3][2];
  d4 = m.m[3][3];

  ans = a1 * det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4) - b1 * det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4) +
        c1 * det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4) - d1 * det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);
  return ans;
}

double det4x4(const TMatrix4 &m) { return det4x4<TMatrix4>(m); }

/*
 *   adjoint( original_matrix, inverse_matrix )
 *
 *     calculate the adjoint of a 4x4 matrix
 *
 *      Let  a   denote the minor determinant of matrix A obtained by
 *           ij
 *
 *      deleting the ith row and jth column from A.
 *
 *                    i+j
 *     Let  b   = (-1)    a
 *          ij            ji
 *
 *    The matrix B = (b  ) is the adjoint of A
 *                     ij
 */

template <class T>
static void adjoint(const T *in, TMatrix4D *out)
{
  double a1, a2, a3, a4, b1, b2, b3, b4;
  double c1, c2, c3, c4, d1, d2, d3, d4;

  /* assign to individual variable names to aid  */
  /* selecting correct values  */

  a1 = in->m[0][0];
  b1 = in->m[0][1];
  c1 = in->m[0][2];
  d1 = in->m[0][3];

  a2 = in->m[1][0];
  b2 = in->m[1][1];
  c2 = in->m[1][2];
  d2 = in->m[1][3];

  a3 = in->m[2][0];
  b3 = in->m[2][1];
  c3 = in->m[2][2];
  d3 = in->m[2][3];

  a4 = in->m[3][0];
  b4 = in->m[3][1];
  c4 = in->m[3][2];
  d4 = in->m[3][3];


  /* row column labeling reversed since we transpose rows & columns */

  out->m[0][0] = det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
  out->m[1][0] = -det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
  out->m[2][0] = det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
  out->m[3][0] = -det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

  out->m[0][1] = -det3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4);
  out->m[1][1] = det3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4);
  out->m[2][1] = -det3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4);
  out->m[3][1] = det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

  out->m[0][2] = det3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4);
  out->m[1][2] = -det3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4);
  out->m[2][2] = det3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4);
  out->m[3][2] = -det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4);

  out->m[0][3] = -det3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3);
  out->m[1][3] = det3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3);
  out->m[2][3] = -det3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3);
  out->m[3][3] = det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3);
}


/*
 *   inverse( original_matrix, inverse_matrix )
 *
 *    calculate the inverse of a 4x4 matrix
 *
 *     -1
 *     A  = ___1__ adjoint A
 *         det A
 */

bool is_invertible(const TMatrix4 &mat) { return fabs(det4x4(mat)) > 1e-15f; }

bool inverse44(const TMatrix4 &in, TMatrix4 &result, float &det)
{
  int i, j;

  /* calculate the adjoint matrix */

  TMatrix4D temp;
  adjoint(&in, &temp);

  /*  calculate the 4x4 determinant
   *  if the determinant is zero,
   *  then the inverse matrix is not unique.
   */

  double detd = det4x4(in);

  if (fabs(detd) < 1.0e-15f)
    return false;
  det = detd;

  /* scale the adjoint matrix to get the inverse */

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      result.m[i][j] = temp.m[i][j] / detd;

  return true;
}

bool inverse44(const TMatrix4D &in, TMatrix4D &result, double &det)
{
  TMatrix4D temp;
  adjoint(&in, &temp);
  double detd = det4x4(in);

  if (fabs(detd) < 1.0e-15f)
    return false;
  det = detd;

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      result.m[i][j] = temp.m[i][j] / detd;

  return true;
}

TMatrix4 inverse44(const TMatrix4 &in)
{
  TMatrix4 result;
  float det;
  if (!inverse44(in, result, det))
  {
    G_ASSERTF(0, "Singular matrix, no inverse!");
    return TMatrix4::IDENT;
  }
  return result;
}


// Builds a matrix that reflects the coordinate system about a plane.
// Plane supposed to be normalized.

TMatrix4 matrix_reflect(const Plane3 &plane)
{
  TMatrix4 result;

  result._11 = -2.f * plane.n.x * plane.n.x + 1.f;
  result._12 = -2.f * plane.n.y * plane.n.x;
  result._13 = -2.f * plane.n.z * plane.n.x;
  result._14 = 0.f;

  result._21 = -2.f * plane.n.x * plane.n.y;
  result._22 = -2.f * plane.n.y * plane.n.y + 1.f;
  result._23 = -2.f * plane.n.z * plane.n.y;
  result._24 = 0.f;

  result._31 = -2.f * plane.n.x * plane.n.z;
  result._32 = -2.f * plane.n.y * plane.n.z;
  result._33 = -2.f * plane.n.z * plane.n.z + 1.f;
  result._34 = 0.f;

  result._41 = -2.f * plane.n.x * plane.d;
  result._42 = -2.f * plane.n.y * plane.d;
  result._43 = -2.f * plane.n.z * plane.d;
  result._44 = 1.f;

  return result;
}

// centerInFrustum = (diff.lengthSq() < afD[2] * afD[2] * (1.f + minSize * minSize / (frustum.zNear * frustum.zNear)));


bool test_triangle_sphere_intersection(const Point3 *triangle, const BSphere3 &sphere)
{
  return test_triangle_sphere_intersection(triangle[0], triangle[1], triangle[2], sphere);
}

bool test_triangle_sphere_intersection(const Point3 &v0, const Point3 &v1, const Point3 &v2, const BSphere3 &sphere)
{
  Point3 kdiff = v0 - sphere.c;
  float a00 = (v1 - v0).lengthSq();
  float a01 = (v1 - v0) * (v2 - v0);
  float a11 = (v2 - v0).lengthSq();
  float b0 = kdiff * (v1 - v0);
  float b1 = kdiff * (v2 - v0);
  float c = kdiff.lengthSq();
  float det = fabsf(a00 * a11 - a01 * a01);
  float s = a01 * b1 - a11 * b0;
  float t = a01 * b0 - a00 * b1;
  float sqrdist;

  if (s + t <= det)
  {
    if (s < 0.f)
    {
      if (t < 0.f)
      {
        if (b0 < 0.f)
        {
          t = 0.f;
          if (-b0 >= a00)
          {
            s = 1.f;
            sqrdist = a00 + 2.f * b0 + c;
          }
          else
          {
            s = -b0 / a00;
            sqrdist = b0 * s + c;
          }
        }
        else
        {
          s = 0.f;
          if (b1 >= 0.f)
          {
            t = 0.f;
            sqrdist = c;
          }
          else if (-b1 >= a11)
          {
            t = 1.f;
            sqrdist = a11 + 2.f * b1 + c;
          }
          else
          {
            t = -b1 / a11;
            sqrdist = b1 * t + c;
          }
        }
      }
      else
      {
        s = 0.f;
        if (b1 >= 0.f)
        {
          t = 0.f;
          sqrdist = c;
        }
        else if (-b1 >= a11)
        {
          t = 1.f;
          sqrdist = a11 + 2.f * b1 + c;
        }
        else
        {
          t = -b1 / a11;
          sqrdist = b1 * t + c;
        }
      }
    }
    else if (t < 0.f)
    {
      t = 0.f;
      if (b0 >= 0.f)
      {
        s = 0.f;
        sqrdist = c;
      }
      else if (-b0 >= a00)
      {
        s = 1.f;
        sqrdist = a00 + 2.f * b0 + c;
      }
      else
      {
        s = -b0 / a00;
        sqrdist = b0 * s + c;
      }
    }
    else
    {
      float invdet = safeinv(det);
      s *= invdet;
      t *= invdet;
      sqrdist = s * (a00 * s + a01 * t + 2.f * b0) + t * (a01 * s + a11 * t + 2.f * b1) + c;
    }
  }
  else
  {
    float tmp0, tmp1, numer, denom;

    if (s < 0.f)
    {
      tmp0 = a01 + b0;
      tmp1 = a11 + b1;
      if (tmp1 > tmp0)
      {
        numer = tmp1 - tmp0;
        denom = a00 - 2.f * a01 + a11;
        if (numer >= denom)
        {
          s = 1.f;
          t = 0.f;
          sqrdist = a00 + 2.f * b0 + c;
        }
        else
        {
          s = numer / denom;
          t = 1.f - s;
          sqrdist = s * (a00 * s + a01 * t + 2.f * b0) + t * (a01 * s + a11 * t + 2.f * b1) + c;
        }
      }
      else
      {
        s = 0.f;
        if (tmp1 <= 0.f)
        {
          t = 1.f;
          sqrdist = a11 + 2.f * b1 + c;
        }
        else if (b1 >= 0.f)
        {
          t = 0.f;
          sqrdist = c;
        }
        else
        {
          t = -b1 / a11;
          sqrdist = b1 * t + c;
        }
      }
    }
    else if (t < 0.f)
    {
      tmp0 = a01 + b1;
      tmp1 = a00 + b0;
      if (tmp1 > tmp0)
      {
        numer = tmp1 - tmp0;
        denom = a00 - 2.f * a01 + a11;
        if (numer >= denom)
        {
          t = 1.f;
          s = 0.f;
          sqrdist = a11 + 2.f * b1 + c;
        }
        else
        {
          t = numer / denom;
          s = 1.f - t;
          sqrdist = s * (a00 * s + a01 * t + 2.f * b0) + t * (a01 * s + a11 * t + 2.f * b1) + c;
        }
      }
      else
      {
        t = 0.f;
        if (tmp1 <= 0.f)
        {
          s = 1.f;
          sqrdist = a00 + 2.f * b0 + c;
        }
        else if (b0 >= 0.f)
        {
          s = 0.f;
          sqrdist = c;
        }
        else
        {
          s = -b0 / a00;
          sqrdist = b0 * s + c;
        }
      }
    }
    else
    {
      numer = a11 + b1 - a01 - b0;
      if (numer <= 0.f)
      {
        s = 0.f;
        t = 1.f;
        sqrdist = a11 + 2.f * b1 + c;
      }
      else
      {
        denom = a00 - 2.f * a01 + a11;
        if (numer >= denom)
        {
          s = 1.f;
          t = 0.f;
          sqrdist = a00 + 2.f * b0 + c;
        }
        else
        {
          s = numer / denom;
          t = 1.f - s;
          sqrdist = s * (a00 * s + a01 * t + 2.f * b0) + t * (a01 * s + a11 * t + 2.f * b1) + c;
        }
      }
    }
  }

  return fabsf(sqrdist) < sphere.r2;
}

static bool test_line_circle_intersection(const Point2 &start, const Point2 &dir, const Point2 &center, float radius, Point2 &out_t)
{
  const Point2 dirFromCenter = center - start;
  const float dist2Sq = dir.lengthSq();
  if (dist2Sq < FLT_EPSILON)
  {
    out_t.x = out_t.y = 0.0f;
    return true;
  }
  else
  {
    float dirsDot = dir * dirFromCenter;
    float segmentDirProjSq = sqr(dirsDot) / dist2Sq;
    float heightSq = dirFromCenter.lengthSq() - segmentDirProjSq;
    if (heightSq > sqr(radius))
      return false;
    const float segmentDirProj = dirsDot >= 0.f ? sqrtf(segmentDirProjSq) : -sqrtf(segmentDirProjSq);
    const float distCenterToContactPoint = sqrtf(sqr(radius) - heightSq);
    out_t.x = segmentDirProj - distCenterToContactPoint;
    out_t.y = segmentDirProj + distCenterToContactPoint;
    return true;
  }
}

static bool test_segment_cylinder_intersection(const Point3 &p0, const Point3 &p1, const Point3 &cyl_left, const Point3 &cyl_up,
  const Point3 &cyl_start, float cyl_radius, const Point3 &cyl_dir_norm, float cyl_len)
{
  Point3 dir = p1 - p0;
  const float dist = max(dir.length(), 0.01f);
  dir /= dist;
  const Point2 p02(p0 * cyl_left, p0 * cyl_up);
  const Point2 p12(p1 * cyl_left, p1 * cyl_up);
  const Point2 center(cyl_start * cyl_left, cyl_start * cyl_up);
  const Point2 dir2 = p12 - p02;
  Point2 t;
  if (!test_line_circle_intersection(p02, dir2, center, cyl_radius, t))
    return false;

  const float dist2Sq = dir2.lengthSq();
  if (t.y < 0.0f)
    return false;
  else if (rabs(t.x) * t.x > dist2Sq)
    return false;
  else if (t.x >= 0.0f)
  {
    Point3 pIn = p0 + dir * t.x;
    float pInProj = (pIn - cyl_start) * cyl_dir_norm;
    return pInProj > 0.f && pInProj < cyl_len;
  }
  else if (rabs(t.y) * t.y < dist2Sq)
  {
    Point3 pOut = p0 + dir * t.y;
    float pOutProj = (pOut - cyl_start) * cyl_dir_norm;
    return pOutProj > 0.f && pOutProj < cyl_len;
  }
  else
  {
    const Point2 range((p0 - cyl_start) * cyl_dir_norm, (p1 - cyl_start) * cyl_dir_norm);
    return max(range.x, range.y) > 0.0f && min(range.x, range.y) < cyl_len;
  }
  return false;
}

bool test_triangle_cylinder_intersection(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &p0, const Point3 &p1,
  float radius)
{
  Point3 dir = p1 - p0;
  const float lenSq = max(dir.lengthSq(), 0.1f);
  const float len = sqrtf(lenSq);
  dir /= len;

  Point3 up = normalizeDef(dir + Point3(1.0f, 1.0f, 1.0f), Point3(0.0f, 1.0f, 0.0f));
  const Point3 left = normalize(dir % up);
  up = left % dir;

  if (test_segment_cylinder_intersection(v0, v1, left, up, p0, radius, dir, len))
    return true;
  if (test_segment_cylinder_intersection(v0, v2, left, up, p0, radius, dir, len))
    return true;
  if (test_segment_cylinder_intersection(v1, v2, left, up, p0, radius, dir, len))
    return true;

  const Point3 trgCenter = (v0 + v1 + v2) * 0.333f;
  const Point3 trgNorm = normalize((v1 - v0) % (v2 - v0));
  float t = -safediv(trgNorm * (p0 - trgCenter), trgNorm * dir);
  return t > 0.0f && t < len;
}

bool test_segment_cylinder_intersection(const Point3 &p0, const Point3 &p1, const Point3 &cylinder_p0, const Point3 &cylinder_p1,
  float cylinder_radius)
{
  Point3 dir = cylinder_p1 - cylinder_p0;
  const float lenSq = max(dir.lengthSq(), 0.1f);
  const float len = sqrtf(lenSq);
  dir /= len;

  Point3 up = normalizeDef(dir + Point3(1.0f, 1.0f, 1.0f), Point3(0.0f, 1.0f, 0.0f));
  const Point3 left = normalize(dir % up);
  up = left % dir;

  return test_segment_cylinder_intersection(p0, p1, left, up, cylinder_p0, cylinder_radius, dir, len);
}
