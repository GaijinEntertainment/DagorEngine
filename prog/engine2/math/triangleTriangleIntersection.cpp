/* Triangle/triangle intersection test routine,
 * by Tomas Moller, 1997.
 * See article "A Fast Triangle-Triangle Intersection Test",
 * Journal of Graphics Tools, 2(2), 1997
 *
 * Updated June 1999: removed the divisions -- a little faster now!
 * Updated October 1999: added {} to CROSS and SUB macros
 *
 * int NoDivTriTriIsect(float V0[3],float V1[3],float V2[3],
 *                      float U0[3],float U1[3],float U2[3])
 *
 * parameters: vertices of triangle 1: V0,V1,V2
 *             vertices of triangle 2: U0,U1,U2
 * result    : returns 1 if the triangles intersect, otherwise 0
 *
 */


#include <math/dag_triangleTriangleIntersection.h>
#include <math/dag_traceRayTriangle.h>


#define FABS(x) (float(fabs(x))) /* implement as is fastest on your machine */

/* if USE_EPSILON_TEST is true then we do a check:
         if |dv|<EPSILON then dv=0.0;
   else no check is done (which is less robust)
*/
#define USE_EPSILON_TEST 1
#define EPSILON          0.000001f


/* some macros */
#define CROSS(dest, v1, v2)                  \
  {                                          \
    dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
    dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
    dest[2] = v1[0] * v2[1] - v1[1] * v2[0]; \
  }

#define DOT(v1, v2) (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])

#define SUB(dest, v1, v2)    \
  {                          \
    dest[0] = v1[0] - v2[0]; \
    dest[1] = v1[1] - v2[1]; \
    dest[2] = v1[2] - v2[2]; \
  }

/* sort so that a<=b */
#define SORT(a, b) \
  if (a > b)       \
  {                \
    float tc;      \
    tc = a;        \
    a = b;         \
    b = tc;        \
  }


/* this edge to edge test is based on Franlin Antonio's gem:
   "Faster Line Segment Intersection", in Graphics Gems III,
   pp. 199-202 */
#define EDGE_EDGE_TEST(V0, U0, U1)                                \
  Bx = U0[i0] - U1[i0];                                           \
  By = U0[i1] - U1[i1];                                           \
  Cx = V0[i0] - U0[i0];                                           \
  Cy = V0[i1] - U0[i1];                                           \
  f = Ay * Bx - Ax * By;                                          \
  d = By * Cx - Bx * Cy;                                          \
  if ((f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f)) \
  {                                                               \
    e = Ax * Cy - Ay * Cx;                                        \
    if (f > 0)                                                    \
    {                                                             \
      if (e >= 0 && e <= f)                                       \
        return 1;                                                 \
    }                                                             \
    else                                                          \
    {                                                             \
      if (e <= 0 && e >= f)                                       \
        return 1;                                                 \
    }                                                             \
  }

#define EDGE_AGAINST_TRI_EDGES(V0, V1, U0, U1, U2) \
  {                                                \
    float Ax, Ay, Bx, By, Cx, Cy, e, d, f;         \
    Ax = V1[i0] - V0[i0];                          \
    Ay = V1[i1] - V0[i1];                          \
    /* test edge U0,U1 against V0,V1 */            \
    EDGE_EDGE_TEST(V0, U0, U1);                    \
    /* test edge U1,U2 against V0,V1 */            \
    EDGE_EDGE_TEST(V0, U1, U2);                    \
    /* test edge U2,U1 against V0,V1 */            \
    EDGE_EDGE_TEST(V0, U2, U0);                    \
  }

#define POINT_IN_TRI(V0, U0, U1, U2)          \
  {                                           \
    float a, b, c, d0, d1, d2;                \
    /* is T1 completly inside T2? */          \
    /* check if V0 is inside tri(U0,U1,U2) */ \
    a = U1[i1] - U0[i1];                      \
    b = -(U1[i0] - U0[i0]);                   \
    c = -a * U0[i0] - b * U0[i1];             \
    d0 = a * V0[i0] + b * V0[i1] + c;         \
                                              \
    a = U2[i1] - U1[i1];                      \
    b = -(U2[i0] - U1[i0]);                   \
    c = -a * U1[i0] - b * U1[i1];             \
    d1 = a * V0[i0] + b * V0[i1] + c;         \
                                              \
    a = U0[i1] - U2[i1];                      \
    b = -(U0[i0] - U2[i0]);                   \
    c = -a * U2[i0] - b * U2[i1];             \
    d2 = a * V0[i0] + b * V0[i1] + c;         \
    if (d0 * d1 > 0.0)                        \
    {                                         \
      if (d0 * d2 > 0.0)                      \
        return 1;                             \
    }                                         \
  }

static int coplanar_tri_tri(const Point3 &normal, const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &u0,
  const Point3 &u1, const Point3 &u2)
{
  float A[3];
  int i0, i1;
  /* first project onto an axis-aligned plane, that maximizes the area */
  /* of the triangles, compute indices: i0,i1. */
  A[0] = FABS(normal[0]);
  A[1] = FABS(normal[1]);
  A[2] = FABS(normal[2]);
  if (A[0] > A[1])
  {
    if (A[0] > A[2])
    {
      i0 = 1; /* A[0] is greatest */
      i1 = 2;
    }
    else
    {
      i0 = 0; /* A[2] is greatest */
      i1 = 1;
    }
  }
  else /* A[0]<=A[1] */
  {
    if (A[2] > A[1])
    {
      i0 = 0; /* A[2] is greatest */
      i1 = 1;
    }
    else
    {
      i0 = 0; /* A[1] is greatest */
      i1 = 2;
    }
  }

  /* test all edges of triangle 1 against the edges of triangle 2 */
  EDGE_AGAINST_TRI_EDGES(v0, v1, u0, u1, u2);
  EDGE_AGAINST_TRI_EDGES(v1, v2, u0, u1, u2);
  EDGE_AGAINST_TRI_EDGES(v2, v0, u0, u1, u2);

  /* finally, test if tri1 is totally contained in tri2 or vice versa */
  POINT_IN_TRI(v0, u0, u1, u2);
  POINT_IN_TRI(u0, v0, v1, v2);

  return 0;
}


#define NEWCOMPUTE_INTERVALS(VV0, VV1, VV2, D0, D1, D2, D0D1, D0D2, A, B, C, X0, X1) \
  {                                                                                  \
    if (D0D1 > 0.0f)                                                                 \
    {                                                                                \
      /* here we know that D0D2<=0.0 */                                              \
      /* that is D0, D1 are on the same side, D2 on the other or on the plane */     \
      A = VV2;                                                                       \
      B = (VV0 - VV2) * D2;                                                          \
      C = (VV1 - VV2) * D2;                                                          \
      X0 = D2 - D0;                                                                  \
      X1 = D2 - D1;                                                                  \
    }                                                                                \
    else if (D0D2 > 0.0f)                                                            \
    {                                                                                \
      /* here we know that d0d1<=0.0 */                                              \
      A = VV1;                                                                       \
      B = (VV0 - VV1) * D1;                                                          \
      C = (VV2 - VV1) * D1;                                                          \
      X0 = D1 - D0;                                                                  \
      X1 = D1 - D2;                                                                  \
    }                                                                                \
    else if (D1 * D2 > 0.0f || D0 < 0.0f || D0 > 0.0f)                               \
    {                                                                                \
      /* here we know that d0d1<=0.0 or that D0!=0.0 */                              \
      A = VV0;                                                                       \
      B = (VV1 - VV0) * D0;                                                          \
      C = (VV2 - VV0) * D0;                                                          \
      X0 = D0 - D1;                                                                  \
      X1 = D0 - D2;                                                                  \
    }                                                                                \
    else if (D1 < 0.0f || D1 > 0.0f)                                                 \
    {                                                                                \
      A = VV1;                                                                       \
      B = (VV0 - VV1) * D1;                                                          \
      C = (VV2 - VV1) * D1;                                                          \
      X0 = D1 - D0;                                                                  \
      X1 = D1 - D2;                                                                  \
    }                                                                                \
    else if (D2 < 0.0f || D2 > 0.0f)                                                 \
    {                                                                                \
      A = VV2;                                                                       \
      B = (VV0 - VV2) * D2;                                                          \
      C = (VV1 - VV2) * D2;                                                          \
      X0 = D2 - D0;                                                                  \
      X1 = D2 - D1;                                                                  \
    }                                                                                \
    else                                                                             \
    {                                                                                \
      /* triangles are coplanar */                                                   \
      return coplanar_tri_tri(n1, v0, v1, v2, u0, u1, u2);                           \
    }                                                                                \
  }


bool test_triangle_triangle_intersection_mueller(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &u0,
  const Point3 &u1, const Point3 &u2)

{
  Point3 e1, e2;
  Point3 n1, n2;
  float d1, d2;
  float du0, du1, du2, dv0, dv1, dv2;
  float dd[3];
  float isect1[2], isect2[2];
  float du0du1, du0du2, dv0dv1, dv0dv2;
  int index;
  float vp0, vp1, vp2;
  float up0, up1, up2;
  float bb, cc, max;

  /* compute plane equation of triangle(V0,V1,V2) */
  SUB(e1, v1, v0);
  SUB(e2, v2, v0);
  CROSS(n1, e1, e2);
  d1 = -DOT(n1, v0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0 = DOT(n1, u0) + d1;
  du1 = DOT(n1, u1) + d1;
  du2 = DOT(n1, u2) + d1;

  /* coplanarity robustness check */
#if USE_EPSILON_TEST
  if (FABS(du0) < EPSILON)
    du0 = 0.0;
  if (FABS(du1) < EPSILON)
    du1 = 0.0;
  if (FABS(du2) < EPSILON)
    du2 = 0.0;
#endif
  du0du1 = du0 * du1;
  du0du2 = du0 * du2;

  if (du0du1 > 0.0f && du0du2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return false;                     /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  SUB(e1, u1, u0);
  SUB(e2, u2, u0);
  CROSS(n2, e1, e2);
  d2 = -DOT(n2, u0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0 = DOT(n2, v0) + d2;
  dv1 = DOT(n2, v1) + d2;
  dv2 = DOT(n2, v2) + d2;

#if USE_EPSILON_TEST
  if (FABS(dv0) < EPSILON)
    dv0 = 0.0;
  if (FABS(dv1) < EPSILON)
    dv1 = 0.0;
  if (FABS(dv2) < EPSILON)
    dv2 = 0.0;
#endif

  dv0dv1 = dv0 * dv1;
  dv0dv2 = dv0 * dv2;

  if (dv0dv1 > 0.0f && dv0dv2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return false;                     /* no intersection occurs */

  /* compute direction of intersection line */
  CROSS(dd, n1, n2);

  /* compute and index to the largest component of D */
  max = (float)FABS(dd[0]);
  index = 0;
  bb = (float)FABS(dd[1]);
  cc = (float)FABS(dd[2]);
  if (bb > max)
    max = bb, index = 1;
  if (cc > max)
    max = cc, index = 2;

  /* this is the simplified projection onto L*/
  vp0 = v0[index];
  vp1 = v1[index];
  vp2 = v2[index];

  up0 = u0[index];
  up1 = u1[index];
  up2 = u2[index];

  /* compute interval for triangle 1 */
  float a, b, c, x0, x1;
  NEWCOMPUTE_INTERVALS(vp0, vp1, vp2, dv0, dv1, dv2, dv0dv1, dv0dv2, a, b, c, x0, x1);

  /* compute interval for triangle 2 */
  float d, e, f, y0, y1;
  NEWCOMPUTE_INTERVALS(up0, up1, up2, du0, du1, du2, du0du1, du0du2, d, e, f, y0, y1);

  float xx, yy, xxyy, tmp;
  xx = x0 * x1;
  yy = y0 * y1;
  xxyy = xx * yy;

  tmp = a * xxyy;
  isect1[0] = tmp + b * x1 * yy;
  isect1[1] = tmp + c * x0 * yy;

  tmp = d * xxyy;
  isect2[0] = tmp + e * xx * y1;
  isect2[1] = tmp + f * xx * y0;

  SORT(isect1[0], isect1[1]);
  SORT(isect2[0], isect2[1]);

  if (isect1[1] < isect2[0] || isect2[1] < isect1[0])
    return false;
  return true;
}


bool test_triangle_triangle_intersection(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &u0, const Point3 &u1,
  const Point3 &u2)

{
  real t, u, v;
  t = 1 + EPSILON;
  if (traceRayToTriangleNoCull(u0, u1 - u0, t, v0, v1, v2, u, v))
    return true;
  t = 1 + EPSILON;
  if (traceRayToTriangleNoCull(u0, u2 - u0, t, v0, v1, v2, u, v))
    return true;
  t = 1 + EPSILON;
  if (traceRayToTriangleNoCull(u1, u2 - u1, t, v0, v1, v2, u, v))
    return true;
  t = 1 + EPSILON;
  if (traceRayToTriangleNoCull(v0, v1 - v0, t, u0, u1, u2, u, v))
    return true;
  t = 1 + EPSILON;
  if (traceRayToTriangleNoCull(v0, v2 - v0, t, u0, u1, u2, u, v))
    return true;
  t = 1 + EPSILON;
  if (traceRayToTriangleNoCull(v1, v2 - v1, t, u0, u1, u2, u, v))
    return true;
  return false;
}

static bool VECTORCALL v_edge_against_tri_edges(vec4f v0, vec4f v1, vec3f u0, vec3f u1, vec3f u2)
{
  vec4f a = v_sub(v1, v0);
  vec3f ax = v_splat_x(a);
  vec3f ay = v_splat_y(a);
  vec3f v0x = v_splat_x(v0);
  vec3f v0y = v_splat_y(v0);
  vec3f u0x = v_perm_xyab(v_perm_xaxa(u0, u1), u2);
  vec3f u1x = v_perm_xyab(v_perm_xaxa(u1, u2), u0);
  vec3f u0y = v_perm_xyab(v_perm_xaxa(v_splat_y(u0), v_splat_y(u1)), v_splat_y(u2));
  vec3f u1y = v_perm_xyab(v_perm_xaxa(v_splat_y(u1), v_splat_y(u2)), v_splat_y(u0));
  vec3f bx = v_sub(u0x, u1x);
  vec3f by = v_sub(u0y, u1y);
  vec3f cx = v_sub(v0x, u0x);
  vec3f cy = v_sub(v0y, u0y);
  vec3f f = v_sub(v_mul(ay, bx), v_mul(ax, by));
  vec3f d = v_sub(v_mul(by, cx), v_mul(bx, cy));
  vec3f e = v_sub(v_mul(ax, cy), v_mul(ay, cx));
  vec3f cond1 = v_and(v_and(v_cmp_gt(f, v_zero()), v_cmp_ge(d, v_zero())), v_cmp_le(d, f));
  vec3f cond2 = v_and(v_and(v_cmp_lt(f, v_zero()), v_cmp_le(d, v_zero())), v_cmp_ge(d, f));
  cond1 = v_and(cond1, v_and(v_cmp_ge(e, v_zero()), v_cmp_le(e, f)));
  cond2 = v_and(cond2, v_and(v_cmp_le(e, v_zero()), v_cmp_ge(e, f)));
  return v_signmask(v_or(cond1, cond2)) & (1 | 2 | 4);
}

static bool VECTORCALL v_coplanar_tri_tri(vec3f normal, vec3f v0, vec3f v1, vec3f v2, vec3f u0, vec3f u1, vec3f u2)
{
  vec3f a = v_abs(normal);
  if (v_extract_x(a) > v_extract_y(a) && v_extract_x(a) > v_extract_z(a))
  {
    /* A[0] is greatest */
    v0 = v_perm_yzxx(v0);
    v1 = v_perm_yzxx(v1);
    v2 = v_perm_yzxx(v2);
    u0 = v_perm_yzxx(u0);
    u1 = v_perm_yzxx(u1);
    u2 = v_perm_yzxx(u2);
  }
  else if (v_extract_y(a) > v_extract_x(a) && v_extract_y(a) > v_extract_z(a))
  {
    /* A[1] is greatest */
    v0 = v_perm_xzxz(v0);
    v1 = v_perm_xzxz(v1);
    v2 = v_perm_xzxz(v2);
    u0 = v_perm_xzxz(u0);
    u1 = v_perm_xzxz(u1);
    u2 = v_perm_xzxz(u2);
  }

  /* test all edges of triangle 1 against the edges of triangle 2 */
  if (v_edge_against_tri_edges(v0, v1, u0, u1, u2))
    return true;
  if (v_edge_against_tri_edges(v1, v2, u0, u1, u2))
    return true;
  if (v_edge_against_tri_edges(v2, v0, u0, u1, u2))
    return true;

  /* finally, test if tri1 is totally contained in tri2 or vice versa */
  if (v_is_point_in_triangle_2d(v0, u0, u1, u2))
    return true;
  if (v_is_point_in_triangle_2d(u0, v0, v1, v2))
    return true;

  return false;
}

static void VECTORCALL v_compute_intervals(vec3f vp, vec3f dv012, vec3f dv0dv1, vec3f dv0dv2, vec3f &bca, vec3f &x0x1)
{
  // dv2 < 0.0f || dv2 > 0.0f
  vec3f bca_4 = v_perm_xycd(v_mul(v_sub(vp, v_splat_z(vp)), v_splat_z(dv012)), v_splat_z(vp));
  vec4f x0x1_4 = v_sub(v_splat_z(dv012), dv012);
  vec4f cmp4 = v_cmp_neq(v_splat_z(dv012), v_zero());
  bca = v_sel(bca, bca_4, cmp4);
  x0x1 = v_sel(x0x1, x0x1_4, cmp4);

  // dv1 < 0.0f || dv1 > 0.0f
  vec3f bca_3 = v_perm_xycd(v_mul(v_sub(v_perm_xzxz(vp), v_splat_y(vp)), v_splat_y(dv012)), v_splat_y(vp));
  vec4f x0x1_3 = v_sub(v_splat_y(dv012), v_perm_xzxz(dv012));
  vec4f cmp3 = v_cmp_neq(v_splat_y(dv012), v_zero());
  bca = v_sel(bca, bca_3, cmp3);
  x0x1 = v_sel(x0x1, x0x1_3, cmp3);

  /* here we know that d0d1<=0.0 or that dv0!=0.0 */
  vec3f bca_2 = v_perm_xycd(v_mul(v_sub(v_perm_yzxw(vp), v_splat_x(vp)), v_splat_x(dv012)), v_splat_x(vp));
  vec4f x0x1_2 = v_sub(v_splat_x(dv012), v_perm_yzxw(dv012));
  vec4f cmp2 = v_or(v_cmp_gt(v_mul(v_splat_y(dv012), v_splat_z(dv012)), v_zero()), v_cmp_neq(v_splat_x(dv012), v_zero()));
  bca = v_sel(bca, bca_2, cmp2);
  x0x1 = v_sel(x0x1, x0x1_2, cmp2);

  /* here we know that d0d1<=0.0 */
  vec3f bca_1 = v_perm_xycd(v_mul(v_sub(v_perm_xzxz(vp), v_splat_y(vp)), v_splat_y(dv012)), v_splat_y(vp));
  vec4f x0x1_1 = v_sub(v_splat_y(dv012), v_perm_xzxz(dv012));
  vec4f cmp1 = v_cmp_gt(v_splat_x(dv0dv2), v_zero());
  bca = v_sel(bca, bca_1, cmp1);
  x0x1 = v_sel(x0x1, x0x1_1, cmp1);

  /* here we know that dv0dv2<=0.0 */
  /* that is dv0, dv1 are on the same side, dv2 on the other or on the plane */
  vec3f bca_0 = v_perm_xycd(v_mul(v_sub(vp, v_splat_z(vp)), v_splat_z(dv012)), v_splat_z(vp));
  vec4f x0x1_0 = v_sub(v_splat_z(dv012), dv012);
  vec4f cmp0 = v_cmp_gt(v_splat_x(dv0dv1), v_zero());
  bca = v_sel(bca, bca_0, cmp0);
  x0x1 = v_sel(x0x1, x0x1_0, cmp0);
}

bool VECTORCALL v_test_triangle_triangle_intersection(vec3f v0, vec3f v1, vec3f v2, vec3f u0, vec3f u1, vec3f u2)
{
  /* compute plane equation of triangle(V0,V1,V2) */
  vec3f e1 = v_sub(v1, v0);
  vec3f e2 = v_sub(v2, v0);
  vec3f n1 = v_cross3(e1, e2);
  vec4f d1 = v_dot3(n1, v0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  vec4f du0 = v_dot3_x(n1, u0);
  vec4f du1 = v_dot3_x(n1, u1);
  vec4f du2 = v_dot3_x(n1, u2);
  vec3f du012 = v_perm_xyab(v_perm_xaxa(du0, du1), du2);
  du012 = v_sub(du012, d1);

  /* coplanarity robustness check */
#if USE_EPSILON_TEST
  vec3f mask1 = v_cmp_ge(v_abs(du012), v_splats(EPSILON));
  du012 = v_and(du012, mask1);
#endif
  vec4f du0du1 = v_mul_x(du012, v_splat_y(du012));
  vec4f du0du2 = v_mul_x(du012, v_splat_z(du012));
  if (v_test_vec_x_gt_0(du0du1) && v_test_vec_x_gt_0(du0du2)) /* same sign on all of them + not equal 0 ? */
    return false;                                             /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  e1 = v_sub(u1, u0);
  e2 = v_sub(u2, u0);
  vec3f n2 = v_cross3(e1, e2);
  vec4f d2 = v_dot3(n2, u0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  vec4f dv0 = v_dot3_x(n2, v0);
  vec4f dv1 = v_dot3_x(n2, v1);
  vec4f dv2 = v_dot3_x(n2, v2);
  vec3f dv012 = v_perm_xyab(v_perm_xaxa(dv0, dv1), dv2);
  dv012 = v_sub(dv012, d2);

#if USE_EPSILON_TEST
  vec3f mask2 = v_cmp_ge(v_abs(dv012), v_splats(EPSILON));
  dv012 = v_and(dv012, mask2);
#endif

  vec4f dv0dv1 = v_mul_x(dv012, v_splat_y(dv012));
  vec4f dv0dv2 = v_mul_x(dv012, v_splat_z(dv012));
  if (v_test_vec_x_gt_0(dv0dv1) && v_test_vec_x_gt_0(dv0dv2)) /* same sign on all of them + not equal 0 ? */
    return false;                                             /* no intersection occurs */

  /* compute direction of intersection line */
  vec3f dd = v_abs(v_cross3(n1, n2));

  /* compute mask of largest component of D */
  vec3f ddX = v_splat_x(dd);
  vec3f ddY = v_splat_y(dd);
  vec3f ddZ = v_splat_z(dd);
  vec3f maskX = v_and(v_cmp_ge(ddX, ddY), v_cmp_ge(ddX, ddZ));
  vec3f maskY = v_andnot(maskX, v_cmp_ge(ddY, ddZ));
  vec3f maskZ = v_andnot(maskX, v_cmp_gt(ddZ, ddY));

  /* this is the simplified projection onto L*/
  mat33f vpmat = {v0, v1, v2};
  v_mat33_transpose(vpmat, vpmat);
  vec3f vp = v_or(v_and(vpmat.col0, maskX), v_or(v_and(vpmat.col1, maskY), v_and(vpmat.col2, maskZ)));
  mat33f upmat = {u0, u1, u2};
  v_mat33_transpose(upmat, upmat);
  vec3f up = v_or(v_and(upmat.col0, maskX), v_or(v_and(upmat.col1, maskY), v_and(upmat.col2, maskZ)));

  vec3f allBitsSet = v_cmp_eq(v_zero(), v_zero());

  /* compute interval for triangle 1 */
  vec3f bca = allBitsSet;
  vec3f x0x1 = allBitsSet;
  v_compute_intervals(vp, dv012, dv0dv1, dv0dv2, bca, x0x1);
  if (v_signmask(v_cmp_eqi(v_and(bca, x0x1), allBitsSet)) == (1 | 2 | 4 | 8))
  {
    /* triangles are coplanar */
    return v_coplanar_tri_tri(n1, v0, v1, v2, u0, u1, u2);
  }

  /* compute interval for triangle 2 */
  vec3f efd = allBitsSet;
  vec3f y0y1 = allBitsSet;
  v_compute_intervals(up, du012, du0du1, du0du2, efd, y0y1);
  if (v_signmask(v_cmp_eqi(v_and(efd, y0y1), allBitsSet)) == (1 | 2 | 4 | 8))
  {
    /* triangles are coplanar */
    return v_coplanar_tri_tri(n1, v0, v1, v2, u0, u1, u2);
  }

  // .xy calculations
  vec4f xx = v_mul(x0x1, v_perm_yxxc(x0x1, x0x1));
  vec4f yy = v_mul(y0y1, v_perm_yxxc(y0y1, y0y1));
  vec4f xxyy = v_mul(xx, yy);

  vec4f tmp1 = v_mul(v_splat_z(bca), xxyy);
  vec4f tmp2 = v_mul(v_splat_z(efd), xxyy);
  // .xyzw
  vec4f isect = v_madd(v_perm_xyab(bca, efd),
    v_mul(v_perm_xyab(v_perm_yxxc(x0x1, x0x1), xx), v_perm_xyab(yy, v_perm_yxxc(y0y1, y0y1))), v_perm_xyab(tmp1, tmp2));

  vec4f isect2010 = v_min(v_perm_zxyw(isect), v_rot_1(v_perm_ywyw(isect)));
  vec4f isect1121 = v_max(v_perm_xzxz(isect), v_perm_ywyw(isect));
  vec4f ret = v_cmp_ge(isect1121, isect2010);
  return v_extract_xi(v_cast_vec4i(ret)) & v_extract_yi(v_cast_vec4i(ret));
}
