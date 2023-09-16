#include <math/dag_mathAng.h>
#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_mathUtils.h>
#include <util/dag_globDef.h>

//==============================================================================
void euler_to_quat(real heading, real attitude, real bank, Quat &quat)
{
  vec4f angles = v_make_vec4f(heading, attitude, bank, bank);
  v_stu(&quat.x, v_quat_from_euler(angles));
}

void euler_heading_to_quat(real heading, Quat &quat)
{
  float c1, s1;
  sincos(heading / 2.f, s1, c1);

  quat.w = c1;
  quat.x = 0;
  quat.y = s1;
  quat.z = 0;
}
void euler_attitude_to_quat(real attitude, Quat &quat)
{
  float c2, s2;
  sincos(attitude / 2.f, s2, c2);

  quat.w = c2;
  quat.x = 0;
  quat.y = 0;
  quat.z = s2;
}
void euler_bank_to_quat(real bank, Quat &quat)
{
  float c3, s3;
  sincos(bank / 2.f, s3, c3);

  quat.w = c3;
  quat.x = s3;
  quat.y = 0;
  quat.z = 0;
}
void euler_heading_attitude_to_quat(real heading, real attitude, Quat &quat)
{
  float c1, s1, c2, s2;
  sincos(heading / 2.f, s1, c1);
  sincos(attitude / 2.f, s2, c2);

  quat.w = c1 * c2;
  quat.x = s1 * s2;
  quat.y = s1 * c2;
  quat.z = c1 * s2;
}
void euler_attitude_bank_to_quat(real attitude, real bank, Quat &quat)
{
  float c2, s2, c3, s3;
  sincos(attitude / 2.f, s2, c2);
  sincos(bank / 2.f, s3, c3);

  quat.w = c2 * c3;
  quat.x = c2 * s3;
  quat.y = s2 * s3;
  quat.z = s2 * c3;
}


//==============================================================================
void quat_to_euler(const Quat &quat, real &heading, real &attitude, real &bank)
{
  const real test = quat.x * quat.y + quat.z * quat.w;

  if (test >= 0.49999f)
  {
    heading = 2 * atan2f(quat.x, quat.w);
    attitude = HALFPI;
    bank = 0;
    return;
  }
  else if (test <= -0.49999f)
  {
    heading = -2 * atan2f(quat.x, quat.w);
    attitude = -HALFPI;
    bank = 0;
    return;
  }

  const real sqx = quat.x * quat.x;
  const real sqy = quat.y * quat.y;
  const real sqz = quat.z * quat.z;

  heading = safe_atan2((quat.y * quat.w - quat.x * quat.z), 0.5f - sqy - sqz);
  attitude = unsafe_asin(2.f * test);
  bank = safe_atan2((quat.x * quat.w - quat.y * quat.z), 0.5f - sqx - sqz);
}

void quat_to_euler_fast(const Quat &quat, real &heading, real &attitude, real &bank)
{
  const real test = 2.f * (quat.x * quat.y + quat.z * quat.w);
  const real sqx = quat.x * quat.x;
  const real sqy = quat.y * quat.y;
  const real sqz = quat.z * quat.z;
  vec4f x = v_make_vec4f(quat.y * quat.w - quat.x * quat.z, test, quat.x * quat.w - quat.y * quat.z, quat.x);
  vec4f y = v_make_vec4f(0.5f - sqy - sqz, 1.f + sqrtf(max(0.f, 1.f - test * test)), 0.5f - sqx - sqz, quat.w);
  vec4f h_a_b_ht = v_atan2(x, y);
  alignas(16) float h_a_b_hmax_sc[4];
  v_st(h_a_b_hmax_sc, h_a_b_ht);

  if (test >= 0.99998f)
  {
    heading = 2.f * h_a_b_hmax_sc[3];
    attitude = HALFPI;
    bank = 0;
    return;
  }
  else if (test <= -0.99998f)
  {
    heading = -2.f * h_a_b_hmax_sc[3];
    attitude = -HALFPI;
    bank = 0;
    return;
  }
  heading = h_a_b_hmax_sc[0];
  attitude = 2.f * h_a_b_hmax_sc[1];
  bank = h_a_b_hmax_sc[2];
}

//==============================================================================
void matrix_to_euler(const TMatrix &tm, real &heading, real &attitude, real &bank)
{
  TMatrix m;
  m.setcol(0, normalize(tm.getcol(0)));
  m.setcol(1, normalize(tm.getcol(1)));
  m.setcol(2, normalize(tm.getcol(2)));

  if (m[0][1] > 0.9999)
  {
    heading = safe_atan2(m.m[2][0], m.m[2][2]);
    attitude = HALFPI;
    bank = 0;
    return;
  }
  else if (m[0][1] < -0.9999)
  {
    heading = -safe_atan2(m.m[2][0], m.m[2][2]);
    attitude = -HALFPI;
    bank = 0;
    return;
  }

  heading = safe_atan2(-m.m[0][2], m.m[0][0]);
  attitude = asin(m.m[0][1]);
  bank = safe_atan2(-m.m[2][1], m.m[1][1]);

  //  debug("z = %f, test = (%f, %f, %f)", attitude, test.x, test.y, test.z);
}


//==============================================================================
real get_axis_angle(const TMatrix &tm, int axis /*1-x; 2-y; 3-z*/)
{
  const Point3 vx = normalize(tm.getcol(0));
  const Point3 vy = normalize(tm.getcol(1));
  const Point3 vz = normalize(tm.getcol(2));

  Point3 proj;
  Point3 ort;

  switch (axis)
  {
    case 1:
      proj = normalize(Point3(0, vz.y, vz.z));
      ort = Point3(0, 0, 1);

      if (proj.length() < 0.0001)
      {
        proj = normalize(Point3(0, vy.y, vy.z));
        ort = Point3(0, 1, 0);
      }
      break;

    case 2:
      proj = normalize(Point3(vx.x, 0, vx.z));
      ort = Point3(1, 0, 0);

      if (proj.length() < 0.0001)
      {
        proj = normalize(Point3(vz.x, 0, vz.z));
        ort = Point3(0, 0, 1);
      }
      break;

    case 3:
      proj = normalize(Point3(vy.x, vy.y, 0));
      ort = Point3(0, 1, 0);

      if (proj.length() < 0.0001)
      {
        proj = normalize(Point3(vx.x, vx.y, 0));
        ort = Point3(1, 0, 0);
      }
      break;

    default: return 0;
  };

  return atan2f(length(proj % ort), proj * ort);
}


//==============================================================================
real vectors_angle(const Point3 &v1, const Point3 &v2)
{
  const Point3 a = normalize(v1);
  const Point3 b = normalize(v2);

  return atan2f(length(a % b), a * b);
}


Point2 dir_to_sph_ang(const Point3 &dir)
{
  Point3 wdir = dir;
  wdir.normalize();
  float zenith, azimuth;

  Point2 v(wdir.z, wdir.x);
  if (!float_nonzero(v.length()))
    return Point2(0, PI);
  zenith = atan(wdir.y / v.length());

  if (!float_nonzero(wdir.z))
    azimuth = 0;
  else
    azimuth = atan(wdir.x / wdir.z);
  if (wdir.z < 0)
    azimuth += PI;
  return Point2(azimuth, zenith);
}

Quat dir_to_quat(const Point3 &dir)
{
  Quat res;
  euler_heading_attitude_to_quat(safe_atan2(-dir.z, dir.x), safe_atan2(dir.y, sqrtf(sqr(dir.x) + sqr(dir.z))), res);
  return res;
}


Quat dir_and_up_to_quat(const Point3 &dir, const Point3 &up)
{
  Point3 right = normalize(cross(up, dir));
  Point3 relUp = normalize(cross(dir, right));
  TMatrix tm;
  tm.setcol(0, dir);
  tm.setcol(1, relUp);
  tm.setcol(2, right);
  return Quat(tm);
}

Point3 sph_ang_to_dir(const Point2 &angles)
{
  float azimuth = angles.x;
  float zenith = angles.y;
  const real cosa = cos(zenith);
  const real cosb = cos(azimuth);
  const real sina = sin(zenith);
  const real sinb = sin(azimuth);

  return Point3(cosa * sinb, sina, cosa * cosb);
}

Quat axis_angle_to_quat(Point3 axis, float ang)
{
  axis = normalize(axis);
  const float halfAng = ang / 2.0f;
  const float s = sin(halfAng);
  return Quat(axis.x * s, axis.y * s, axis.z * s, cos(halfAng));
}

void quat_to_axis_angle(const Quat &q_, Point3 &axis, float &ang)
{
  //
  ////  if (q.w > 1)
  ////    q.normalise(); // if w>1 acos and sqrt will produce errors, this cant happen if quaternion is normalised
  //  G_ASSERT(q.w <= 1.0f);

  Quat q = normalize(q_);

  // ang = 2.0f * acosf(q.w);

  if (q.w >= 1.0f)
    ang = 0;
  else if (q.w <= -1.0f)
    ang = 2 * PI;
  else
    ang = 2.0f * acosf(q.w);

  float s = sqrtf(1.0f - q.w * q.w); // assuming quaternion normalised then w is less than 1, so term always positive.
  if (s < 0.001f)                    // test to avoid divide by zero, s is always positive due to sqrt
  {
    // if s close to zero then direction of axis not important
    axis = Point3(1, 0, 0);
  }
  else
  {
    axis.x = q.x / s; // normalise axis
    axis.y = q.y / s;
    axis.z = q.z / s;
  }
}

bool is_direction_clockwise(float angle_1, float angle_2)
{
  const float res = angle_2 - renorm_ang(angle_1, angle_2);
  if (is_equal_float(rabs(res), PI))
    return true;

  return res >= 0.f;
}

bool is_angle_in_sector(float test_angle, const Point2 &sector)
{
  return sector.y > sector.x ? test_angle > sector.x && test_angle < sector.y : test_angle > sector.x || test_angle < sector.y;
}


bool is_sector_intersects_sector(const Point2 &sector_1, const Point2 &sector_2)
{
  if (is_equal_float(sector_2.x, sector_1.x) && is_equal_float(sector_2.y, sector_1.y))
    return false;

  const bool isAngle1InSector1 = is_angle_in_sector(sector_1.x, sector_2);
  const bool isAngle2InSector1 = is_angle_in_sector(sector_1.y, sector_2);

  const bool isAngle1InSector2 = is_angle_in_sector(sector_2.x, sector_1);
  const bool isAngle2InSector2 = is_angle_in_sector(sector_2.y, sector_1);

  return isAngle1InSector1 || isAngle2InSector1 || isAngle1InSector2 || isAngle2InSector2;
}

bool is_direction_clockwise_deg(float angle_1_deg, float angle_2_deg)
{
  return is_direction_clockwise(DegToRad(angle_1_deg), DegToRad(angle_2_deg));
}

bool is_angle_in_sector_deg(float test_angle_deg, const Point2 &sector_deg)
{
  return is_angle_in_sector(DegToRad(test_angle_deg), Point2(DegToRad(sector_deg.x), DegToRad(sector_deg.y)));
}

bool is_sector_intersects_sector_deg(const Point2 &sector_1_deg, const Point2 &sector_2_deg)
{
  return is_sector_intersects_sector(Point2(DegToRad(sector_1_deg.x), DegToRad(sector_1_deg.y)),
    Point2(DegToRad(sector_2_deg.x), DegToRad(sector_2_deg.y)));
}

void sincos(float rad, float &s, float &c)
{
  vec4f vs, vc;
  v_sincos_x(v_set_x(rad), vs, vc);
  s = v_extract_x(vs);
  c = v_extract_x(vc);
}

Point3 basis_aware_angles_to_dir(const Point2 &angles, const Point3 &up, const Point3 &fwd)
{
  Point3 dir = Quat(up, angles.x) * fwd;
  Point3 zaxis = normalize(cross(dir, up));
  return Quat(zaxis, angles.y) * dir;
}

Point2 basis_aware_dir_to_angles(const Point3 &dir, const Point3 &up, const Point3 &fwd)
{
  Point3 dxdz = normalize(dir - up * dot(dir, up));
  float dy = dot(dir, up);
  Point3 root2dxdz = cross(fwd, dxdz);
  float sign = dot(up, root2dxdz) > .0 ? 1.0 : -1.0;
  return Point2(atan2(root2dxdz.length() * sign, dot(dxdz, fwd)), asin(dy));
}