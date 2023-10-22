#include <math/dag_boundingSphere.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_debug.h>

static BSphere3 meshBoundingSphereApprox(const Point3 *point, unsigned point_count)
{
  if (!point || point_count == 0)
    return BSphere3();

  Point3 x_min = point[0];
  Point3 x_max = point[0];
  Point3 y_min = point[0];
  Point3 y_max = point[0];
  Point3 z_min = point[0];
  Point3 z_max = point[0];


  // finds two points that are close to maximally spaced

  const Point3 *pt = point;
  const Point3 *end = point + point_count;

  BBox3 box;

  while (pt < end)
  {
    box += *pt;

    if (pt->x < x_min.x)
      x_min = *pt;
    else if (pt->x > x_max.x)
      x_max = *pt;

    if (pt->y < y_min.y)
      y_min = *pt;
    else if (pt->y > y_max.y)
      y_max = *pt;

    if (pt->z < z_min.z)
      z_min = *pt;
    else if (pt->z > z_max.z)
      z_max = *pt;

    ++pt;
  }


  // pick the pair with the maximum point-to-point separation

  Point3 b1 = x_min;
  Point3 b2 = x_max;

  if (y_max.y - y_min.y > b2.x - b1.x)
  {
    b1 = y_min;
    b2 = y_max;
  }

  if (z_max.z - z_min.z > b2.y - b1.y)
  {
    b1 = z_min;
    b2 = z_max;
  }


  // calculate the initial sphere

  DPoint3 db = dpoint3(b2 - b1);
  DPoint3 half_db = 0.5 * db;
  double r = half_db.length();
  double r2 = r * r;
  DPoint3 center = dpoint3(b1) + half_db;

  // correct sphere if necessary

  pt = point;
  end = point + point_count;

  while (pt < end)
  {
    double dr2 = lengthSq(dpoint3(*pt) - center);

    if (dr2 > r2)
    {
      // point is outside current sphere, update

      double dr = sqrt(dr2);

      r = 0.5 * (r + dr);
      r2 = r * r;

      double delta = dr - r;
      center = (dpoint3(*pt) - center) * (delta / dr) + center;
    }

    ++pt;
  } // for each point

  DPoint3 center_1 = dpoint3(box.center());
  double r_1 = box[1].x - box[0].x;
  double r_2 = r_1 * r_1;

  // correct sphere if necessary

  pt = point;
  end = point + point_count;

  while (pt < end)
  {
    double dr2 = lengthSq(dpoint3(*pt) - center_1);

    if (dr2 > r_2)
    {
      // point is outside current sphere, update

      double dr = sqrt(dr2);

      r_1 = 0.5 * (r_1 + dr);
      r_2 = r_1 * r_1;

      double delta = dr - r_1;
      center_1 = (dpoint3(*pt) - center_1) * (delta / dr) + center_1;
    }

    ++pt;
  } // for each point

  if (r_1 < r)
    return BSphere3(point3(center_1), r_1);

  return BSphere3(point3(center), r);
}


//------------------------------------------------------------------------------

BSphere3 triangle_bounding_sphere(const Point3 &p1, const Point3 &p2, const Point3 &p3)
{
  bool blunt_angled = false;
  Point3 d1;
  Point3 d2;


  // find if triangle is blunt-angled,
  // if so, find longest side

  if ((p2 - p1) * (p3 - p1) < 0)
  {
    blunt_angled = true;

    d1 = p2;
    d2 = p3;
  }
  else if ((p3 - p2) * (p1 - p2) < 0)
  {
    blunt_angled = true;

    d1 = p3;
    d2 = p1;
  }
  else if ((p1 - p3) * (p2 - p3) < 0)
  {
    blunt_angled = true;

    d1 = p1;
    d2 = p2;
  }


  // compute center & radius

  Point3 center;
  float r;

  if (blunt_angled)
  {
    Point3 d = d2 - d1;
    Point3 half_d = 0.5f * d;

    center = d1 + half_d;
    r = half_d.length();
  }
  else
  {
    float nd1 = (p3 - p1) * (p2 - p1);
    float nd2 = (p3 - p2) * (p1 - p2);
    float nd3 = (p1 - p3) * (p2 - p3);
    float c1 = nd2 * nd3;
    float c2 = nd3 * nd1;
    float c3 = nd1 * nd2;
    float c = c1 + c2 + c3;

    r = 0.5f * sqrt(fabsf(safediv((nd1 + nd2) * (nd2 + nd3) * (nd3 + nd1), c)));
    center = (p1 * (c2 + c3) + p2 * (c3 + c1) + p3 * (c1 + c2)) * safeinv(2 * c);
  }

  return BSphere3(center, r);
}

//==============================================================================

class Sphere3
{
public:
  Sphere3();
  Sphere3(const DPoint3 &o);
  Sphere3(const DPoint3 &o, double r);
  Sphere3(const DPoint3 &o, const DPoint3 &a);
  Sphere3(const DPoint3 &O, const DPoint3 &a, const DPoint3 &b);
  Sphere3(const DPoint3 &O, const DPoint3 &a, const DPoint3 &b, const DPoint3 &c);

  const DPoint3 &center() const;
  double radius() const;

  double d2(const DPoint3 &pt) const; // square distance from p to boundary of the Sphere

  static Sphere3 miniBall(const Point3 *point, unsigned int point_count);

  static const double radiusEpsilon;

  static bool isValid() { return MiniballIsValid_; }

private:
  static Sphere3 recurseMini_(const Point3 **point, unsigned int point_count, unsigned int b = 0);


  DPoint3 center_;
  double radius_;


  static bool MiniballIsValid_; // flag used for checks for patalogic meshes
};

const double Sphere3::radiusEpsilon = 0.0001;
bool Sphere3::MiniballIsValid_;


//------------------------------------------------------------------------------

inline const DPoint3 &Sphere3::center() const { return center_; }


//------------------------------------------------------------------------------

inline double Sphere3::radius() const { return radius_; }


//------------------------------------------------------------------------------

inline Sphere3::Sphere3() : radius_(-1), center_(ZERO<DPoint3>()) {}


//------------------------------------------------------------------------------

inline Sphere3::Sphere3(const DPoint3 &o) : radius_(0 + radiusEpsilon), center_(o) {}


//------------------------------------------------------------------------------

inline Sphere3::Sphere3(const DPoint3 &o, double r) : radius_(r), center_(o) {}


//------------------------------------------------------------------------------

inline Sphere3::Sphere3(const DPoint3 &pt1, const DPoint3 &pt2)
{
  DPoint3 db = pt2 - pt1;
  DPoint3 half_db = 0.5 * db;

  radius_ = half_db.length() + radiusEpsilon;
  center_ = pt1 + half_db;
}


//------------------------------------------------------------------------------

inline Sphere3::Sphere3(const DPoint3 &org, const DPoint3 &a, const DPoint3 &b)
{
  DPoint3 da = a - org;
  DPoint3 db = b - org;
  float d = 2.0f * ((da % db) * (da % db));

  if (fabs(d) > 0.000001)
  {
    DPoint3 c = ((db * db) * ((da % db) % da) + (da * da) * (db % (da % db))) / d;

    radius_ = c.length() + radiusEpsilon;
    center_ = org + c;
  }
  else
  {
    MiniballIsValid_ = false;
  }
}


//------------------------------------------------------------------------------

static inline double Matrix_det(double m11, double m12, double m13, double m21, double m22, double m23, double m31, double m32,
  double m33)
{
  return m11 * (m22 * m33 - m32 * m23) - m21 * (m12 * m33 - m32 * m13) + m31 * (m12 * m23 - m22 * m13);
}


//------------------------------------------------------------------------------

inline Sphere3::Sphere3(const DPoint3 &org, const DPoint3 &a, const DPoint3 &b, const DPoint3 &c)
{
  DPoint3 da = dpoint3(a - org);
  DPoint3 db = dpoint3(b - org);
  DPoint3 dc = dpoint3(c - org);

  double d = 2.0 * Matrix_det(da.x, da.y, da.z, db.x, db.y, db.z, dc.x, dc.y, dc.z);
  if (fabs(d) > 0.000001)
  {
    DPoint3 o = (((dc * dc) * (da % db) + (db * db) * (dc % da) + (da * da) * (db % dc)) / d);

    radius_ = o.length() + radiusEpsilon;
    center_ = org + o;
  }
  else
  {
    //        G_ASSERT(fabs(d) > 0.0001)
    MiniballIsValid_ = false;
  }
}


//------------------------------------------------------------------------------

inline double Sphere3::d2(const DPoint3 &pt) const { return (pt - center_) * (pt - center_) - radius_ * radius_; }


//------------------------------------------------------------------------------

Sphere3 Sphere3::recurseMini_(const Point3 **point, unsigned int point_count, unsigned int b)
{
  if (!MiniballIsValid_)
    return Sphere3();


  Sphere3 mb;

  switch (b)
  {
    case 0: mb = Sphere3(); break;

    case 1: mb = Sphere3(dpoint3(*point[-1])); break;

    case 2: mb = Sphere3(dpoint3(*point[-1]), dpoint3(*point[-2])); break;

    case 3: mb = Sphere3(dpoint3(*point[-1]), dpoint3(*point[-2]), dpoint3(*point[-3])); break;

    case 4: mb = Sphere3(dpoint3(*point[-1]), dpoint3(*point[-2]), dpoint3(*point[-3]), dpoint3(*point[-4])); return mb;
  }

  if (!MiniballIsValid_)
    return Sphere3();

  for (unsigned int i = 0; i < point_count; i++)
  {
    if (mb.radius() < 0 || mb.d2(dpoint3(*point[i])) >= 0) // signed square distance to sphere
    {
      for (unsigned j = i; j > 0; j--)
      {
        const Point3 *t = point[j];

        point[j] = point[j - 1];
        point[j - 1] = t;
      }

      mb = recurseMini_(point + 1, i, b + 1);
    }
  }

  return mb;
}


//------------------------------------------------------------------------------

inline Sphere3 Sphere3::miniBall(const Point3 *point, unsigned point_count)
{
  const Point3 **pt = new const Point3 *[point_count];

  for (unsigned int i = 0; i < point_count; i++)
    pt[i] = &point[i];


  MiniballIsValid_ = true;

  Sphere3 mb = recurseMini_(pt, point_count);
  delete[] pt;

  if (mb.radius() < 0)
    MiniballIsValid_ = false;

  if (!MiniballIsValid_)
  {
    // printf( "\nminiball failed, using approximation, %d points", point_count );
  }

  return mb;
}


//==============================================================================

static BSphere3 meshAlmostBoundingSphere(const Point3 *point, unsigned point_count)
{
  Sphere3 mb = Sphere3::miniBall(point, point_count);

  if (!mb.isValid())
  {
    BSphere3 bound = ::meshBoundingSphereApprox(point, point_count);
    return bound;
  }
  return BSphere3(point3(mb.center()), mb.radius());
}

BSphere3 mesh_bounding_sphere(const Point3 *point, unsigned point_count)
{
  BBox3 box;
  for (int i = 0; i < point_count; ++i)
    box += point[i];
  BSphere3 sph = ::meshAlmostBoundingSphere(point, point_count);
  if (sph.r < 0)
  {
    sph.setempty();
    sph.c = box.center();
  }
  Point3 bc = box.center();
  float br2 = 0;
  for (; point_count; point_count--, point++)
  {
    // ensure is bounding - floating point error safety
    float dr2 = lengthSq(*point - sph.c);
    br2 = max(br2, lengthSq(*point - bc));
    if (dr2 > sph.r2)
      sph.r2 = dr2;
  }
  if (br2 < sph.r2)
  {
    sph.r2 = br2;
    sph.c = bc;
  }
  sph.r = sqrtf(sph.r2);
  return sph;
}

static vec4f meshBoundingSphereApprox(const Point3 *point, unsigned point_count, bbox3f_cref box)
{
  if (!point || point_count == 0)
    return V_C_MIN_VAL;
  Point3_vec4 x_min = point[0];
  Point3_vec4 x_max = point[0];
  Point3_vec4 y_min = point[0];
  Point3_vec4 y_max = point[0];
  Point3_vec4 z_min = point[0];
  Point3_vec4 z_max = point[0];


  // finds two points that are close to maximally spaced

  const Point3 *pt = point;
  const Point3 *const end = point + point_count;

  while (pt < end)
  {
    if (pt->x < x_min.x)
      x_min = *pt;
    else if (pt->x > x_max.x)
      x_max = *pt;

    if (pt->y < y_min.y)
      y_min = *pt;
    else if (pt->y > y_max.y)
      y_max = *pt;

    if (pt->z < z_min.z)
      z_min = *pt;
    else if (pt->z > z_max.z)
      z_max = *pt;

    ++pt;
  }


  // pick the pair with the maximum point-to-point separation

  Point3_vec4 b1 = x_min;
  Point3_vec4 b2 = x_max;

  if (y_max.y - y_min.y > b2.x - b1.x)
  {
    b1 = y_min;
    b2 = y_max;
  }

  if (z_max.z - z_min.z > b2.y - b1.y)
  {
    b1 = z_min;
    b2 = z_max;
  }


  // calculate the initial sphere

  Point3_vec4 db = (b2 - b1);
  Point3_vec4 half_db = 0.5 * db;
  float r = length(half_db);
  float r2 = r * r;
  Point3_vec4 center = b1 + half_db;

  // correct sphere if necessary

  pt = point;

  while (pt < end)
  {
    float dr2 = lengthSq(*pt - center);

    if (dr2 > r2)
    {
      // point is outside current sphere, update

      float dr = sqrtf(dr2);

      r = 0.5 * (r + dr);
      r2 = r * r;

      float delta = dr - r;
      center += (*pt - center) * (delta / dr);
    }

    ++pt;
  } // for each point

  Point3_vec4 center_1;
  v_st(&center_1.x, v_bbox3_center(box));
  float r_1 = v_extract_x(box.bmax) - v_extract_x(box.bmin);
  float r_2 = r_1 * r_1;

  // correct sphere if necessary
  pt = point;

  while (pt < end)
  {
    float dr2 = lengthSq(*pt - center_1);

    if (dr2 > r_2)
    {
      // point is outside current sphere, update

      float dr = sqrtf(dr2);

      r_1 = 0.5f * (r_1 + dr);
      r_2 = r_1 * r_1;

      float delta = dr - r_1;
      center_1 += (*pt - center_1) * (delta / dr);
    }

    ++pt;
  } // for each point

  if (r_2 < r2)
    return v_make_vec4f(center_1.x, center_1.y, center_1.z, r_2);

  return v_make_vec4f(center.x, center.y, center.z, r2);
}

vec4f mesh_fast_bounding_sphere(const Point3 *point, unsigned point_count, bbox3f_cref box)
{
  vec4f sph_c_hint = v_bbox3_center(box);
  vec4f sph = meshBoundingSphereApprox(point, point_count, box);
  if (v_extract_w(sph) < 0)
    sph = sph_c_hint;
  vec3f distHint = v_zero(), distSph = v_zero();
  for (; point_count; point_count--, point++)
  {
    // ensure is bounding - floating point error safety
    vec3f pt = v_ldu(&point->x);
    distSph = v_max(distSph, v_length3_sq_x(v_sub(pt, sph)));
    distHint = v_max(distHint, v_length3_sq_x(v_sub(pt, sph_c_hint)));
  }
  sph = v_perm_xyzd(sph, v_splat_x(distSph));
  if (v_extract_x(distHint) < v_extract_x(distSph)) // check if box center is better center
    sph = v_perm_xyzd(sph_c_hint, v_splat_x(distHint));

  return sph;
}

BSphere3 mesh_fast_bounding_sphere(const Point3 *point, unsigned point_count)
{
  bbox3f box;
  v_bbox3_init_empty(box);
  for (const Point3 *i = point, *e = point + point_count; i != e; ++i)
    v_bbox3_add_pt(box, v_ldu(&i->x));
  vec4f sph = mesh_fast_bounding_sphere(point, point_count, box);
  return BSphere3(Point3(v_extract_x(sph), v_extract_y(sph), v_extract_z(sph)), sqrtf(v_extract_w(sph)));
}
