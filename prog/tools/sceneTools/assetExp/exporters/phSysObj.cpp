#include "phSysObj.h"
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>


bool PhysSysObject::load(Mesh &m, const TMatrix &wtm, const DataBlock &blk)
{
  mesh.init(m);
  matrix = wtm;

  materialName = blk.getStr("phmat", "");

  const char *s = blk.getStr("collType", blk.getStr("collision", ""));
  if (!blk.getBool("collidable", true))
    s = "none";

  if (stricmp(s, "none") == 0)
    collType = COLL_NONE;
  else if (stricmp(s, "box") == 0)
    collType = COLL_BOX;
  else if (stricmp(s, "sphere") == 0)
    collType = COLL_SPHERE;
  else if (stricmp(s, "capsule") == 0)
    collType = COLL_CAPSULE;
  else
  {
    debug("bad collType <%s>", s);
    return false;
  }

  s = blk.getStr("massType", "none");
  if (stricmp(s, "none") == 0)
    massType = MASS_NONE;
  else if (stricmp(s, "box") == 0)
    massType = MASS_BOX;
  else if (stricmp(s, "sphere") == 0)
    massType = MASS_SPHERE;
  else if (stricmp(s, "mesh") == 0)
    massType = MASS_MESH;
  else
  {
    debug("bad massType <%s>", s);
    return false;
  }

  if (blk.paramExists("preferMass"))
  {
    useMass = blk.getBool("preferMass", false);
    massDens = blk.getReal(useMass ? "mass" : "density", -1);
  }
  else
  {
    massDens = blk.getReal("density", -1);
    if (massDens < 0)
    {
      massDens = blk.getReal("mass", -1);
      useMass = true;
    }
    else
      useMass = false;
  }
  return true;
}

bool PhysSysObject::calcMomj(PhysSysObject::Momj &mj)
{
  real volume;
  Matrix3 momj;
  Point3 cmPos;

  TMatrix wtm = matrix;

  if (massType == MASS_BOX)
  {
    BBox3 box;
    if (!getBoxCollision(box))
      return false;

    Point3 size = box.width();
    size.x *= wtm.getcol(0).length();
    size.y *= wtm.getcol(1).length();
    size.z *= wtm.getcol(2).length();

    cmPos = wtm * box.center();

    momj.zero();
    momj[0][0] = (size.y * size.y + size.z * size.z) / 12;
    momj[1][1] = (size.x * size.x + size.z * size.z) / 12;
    momj[2][2] = (size.y * size.y + size.x * size.x) / 12;

    // transform momj to world space
    Matrix3 ntm;
    ntm.setcol(0, normalize(wtm.getcol(0)));
    ntm.setcol(1, normalize(wtm.getcol(1)));
    ntm.setcol(2, normalize(wtm.getcol(2)));

    if (ntm.det() < 0)
      ntm.setcol(2, -ntm.getcol(2));

    momj = ntm * momj * inverse(ntm);

    volume = size.x * size.y * size.z;
  }
  else if (massType == MASS_SPHERE)
  {
    Point3 center;
    real radius;
    if (!getSphereCollision(center, radius))
      return false;

    cmPos = wtm * center;

    radius *= (wtm.getcol(0).length() + wtm.getcol(1).length() + wtm.getcol(2).length()) / 3;

    momj.zero();
    momj[0][0] = momj[1][1] = momj[2][2] = 0.4f * radius * radius;

    volume = radius * radius * radius * PI * 4 / 3;
  }
  else if (massType == MASS_MESH)
  {
    if (!mesh.faces.size())
      return false;

    mesh.calcMomj(momj, cmPos, volume, wtm);

    mj.cmPos = cmPos;

    if (useMass)
    {
      mj.mass = massDens;

      if (volume == 0)
        mj.momj.zero();
      else
        mj.momj = momj * (massDens / volume);
    }
    else
    {
      mj.mass = massDens * volume;
      mj.momj = momj * massDens;
    }

    return true;
  }
  else
  {
    return false;
  }

  mj.cmPos = cmPos;

  if (useMass)
  {
    mj.mass = massDens;
    mj.momj = momj * massDens;
  }
  else
  {
    mj.mass = massDens * volume;
    mj.momj = momj * mj.mass;
  }

  return true;
}

bool PhysSysObject::getBoxCollision(BBox3 &box)
{
  box = mesh.localBox;
  return !box.isempty();
}

bool PhysSysObject::getSphereCollision(Point3 &center, real &radius)
{
  BBox3 box = mesh.localBox;
  if (box.isempty())
    return false;

  center = box.center();

  Point3 w = box.width();
  radius = (w.x + w.y + w.z) / (3 * 2);

  return true;
}

bool PhysSysObject::getCapsuleCollision(Point3 &center, Point3 &extent, real &radius, const Point3 &scale)
{
  BBox3 box = mesh.localBox;
  if (box.isempty())
    return false;

  center = box.center();

  Point3 w = box.width();
  w.x *= rabs(scale.x);
  w.y *= rabs(scale.y);
  w.z *= rabs(scale.z);

  if (w.x >= w.y && w.x >= w.z)
  {
    extent = Point3(w.x / 2, 0, 0);
    radius = (w.y + w.z) / 4;
  }
  else if (w.y >= w.x && w.y >= w.z)
  {
    extent = Point3(0, w.y / 2, 0);
    radius = (w.x + w.z) / 4;
  }
  else
  {
    extent = Point3(0, 0, w.z / 2);
    radius = (w.x + w.y) / 4;
  }

  return true;
}


static double cubicRoot(double x) { return (x < 0) ? -pow(-x, 1.0 / 3.0) : pow(x, 1.0 / 3.0); }

void PhysSysBodyObject::calcMomjAndTm(dag::ConstSpan<PhysSysObject *> objs)
{
  Tab<PhysSysObject::Momj> momjs(tmpmem);
  momjs.reserve(objs.size());

  for (int i = 0; i < objs.size(); ++i)
  {
    PhysSysObject::Momj mj;
    if (objs[i]->calcMomj(mj))
      momjs.push_back(mj);
  }

  calcBodyMomjAndTm(momjs, false, false, false, false);
}

void PhysSysBodyObject::calcBodyMomjAndTm(dag::ConstSpan<PhysSysObject::Momj> objs, bool force_ident_tm, bool force_cm_posx_0,
  bool force_cm_posy_0, bool force_cm_posz_0)
{
  // find center of mass
  DPoint3 centerOfMass(0, 0, 0);
  double mass = 0;

  for (int i = 0; i < objs.size(); ++i)
  {
    const PhysSysObject::Momj &o = objs[i];

    mass += o.mass;
    centerOfMass += dpoint3(o.cmPos) * o.mass;
  }

  if (mass != 0)
    centerOfMass /= mass;
  else
  {
    centerOfMass = DPoint3(0, 0, 0);

    for (int i = 0; i < objs.size(); ++i)
    {
      const PhysSysObject::Momj &o = objs[i];

      centerOfMass += dpoint3(o.cmPos);
    }

    if (objs.size())
      centerOfMass /= objs.size();
  }

  // if (mass<0) explog("body '%s' has negative mass!\r\n", (const char*)node->GetName());

  // calc total momj
  Matrix3 momj;
  momj.zero();

  for (int i = 0; i < objs.size(); ++i)
  {
    const PhysSysObject::Momj &o = objs[i];

    momj += o.momj;

    DPoint3 r = dpoint3(o.cmPos) - centerOfMass;

    momj[0][0] += o.mass * (r.y * r.y + r.z * r.z);
    momj[1][1] += o.mass * (r.z * r.z + r.x * r.x);
    momj[2][2] += o.mass * (r.x * r.x + r.y * r.y);

    momj[0][1] -= o.mass * r.x * r.y;
    momj[1][0] -= o.mass * r.x * r.y;
    momj[0][2] -= o.mass * r.x * r.z;
    momj[2][0] -= o.mass * r.x * r.z;
    momj[1][2] -= o.mass * r.y * r.z;
    momj[2][1] -= o.mass * r.y * r.z;
  }

  if (force_ident_tm)
  {
    // set body tm and props
    bodyMass = mass;
    bodyMomj = Point3(momj[0][0], momj[1][1], momj[2][2]);

    if (force_cm_posx_0)
      centerOfMass.x = 0;
    if (force_cm_posy_0)
      centerOfMass.y = 0;
    if (force_cm_posz_0)
      centerOfMass.z = 0;

    matrix.identity();
    matrix.setcol(3, centerOfMass);
    return;
  }


  // find inertia ellipsoid axes

  // find eigenvalues
  Point3 dmomj;

  {
    double Mxx = momj[0][0];
    double Myy = momj[1][1];
    double Mzz = momj[2][2];
    double Mxy = momj[0][1];
    double Myz = momj[1][2];
    double Mzx = momj[2][0];

    double a = -1;
    double b = Mxx + Myy + Mzz;
    double c = Mxy * Mxy + Myz * Myz + Mzx * Mzx - Mxx * Myy - Myy * Mzz - Mzz * Mxx;
    double d = Mxx * Myy * Mzz + 2 * Mxy * Myz * Mzx - Mxx * Myz * Myz - Myy * Mzx * Mzx - Mzz * Mxy * Mxy;

    double f = c / a - b * b / (a * a * 3);
    double g = 2 * b * b * b / (a * a * a * 27) - b * c / (a * a) / 3 + d / a;
    double h = g * g / 4 + f * f * f / 27;

    if (f >= 0)
    {
      // 3 equal real roots
      double x1 = ::cubicRoot(-d / a);

      dmomj = Point3(x1, x1, x1);
    }
    else
    {
      // 3 real roots
      double i = sqrt(-f * f * f / 27);
      double j = ::cubicRoot(i);

      double k = -g / (2 * i);
      if (k >= 1)
        k = 0;
      else if (k <= -1)
        k = PI;
      else
        k = acos(k);

      double L = -j;
      double M = cos(k / 3);
      double N = sqrt(3.0f) * sin(k / 3);
      double P = -b / (3 * a);

      double x1 = 2 * j * M + P;
      double x2 = L * (M + N) + P;
      double x3 = L * (M - N) + P;

      dmomj = Point3(x1, x2, x3);
    }
  }

  // find eigenvectors
  Matrix3 dmj;
  dmj.identity();

  for (int i = 0; i < 3; ++i)
  {
    Point3 sm[3];
    sm[0] = momj.getcol(0);
    sm[1] = momj.getcol(1);
    sm[2] = momj.getcol(2);

    sm[0][0] -= dmomj[i];
    sm[1][1] -= dmomj[i];
    sm[2][2] -= dmomj[i];

    sm[0].normalize();
    sm[1].normalize();
    sm[2].normalize();

    const float EPS = 0.001f;

    if (lengthSq(sm[0]) <= EPS)
    {
      if (lengthSq(sm[2]) <= EPS)
      {
        Point3 p = sm[0];
        sm[0] = sm[1];
        sm[1] = p;
      }
      else
      {
        Point3 p = sm[0];
        sm[0] = sm[2];
        sm[2] = p;
      }
    }
    else if (lengthSq(sm[1]) <= EPS)
    {
      Point3 p = sm[1];
      sm[1] = sm[2];
      sm[2] = p;
    }

    Point3 v;

    if (lengthSq(sm[0]) <= EPS)
    {
      // matrix is zero - any vector is ok
      v = Point3(0, 0, 0);
      v[i] = 1;
    }
    else if (lengthSq(sm[1]) <= EPS)
    {
      // two zero rows - any vector orthogonal to non-zero row
      v = sm[0] % Point3(1, 0, 0);
      if (lengthSq(v) < 0.1f)
        v = normalize(sm[0] % Point3(0, 1, 0));
      else
        v = normalize(v);
    }
    else
    {
      v = sm[0] % sm[1];
      if (lengthSq(v) <= EPS)
      {
        // sm[0] and sm[1] are collinear

        if (lengthSq(sm[2]) <= EPS)
        {
          // any vector orthogonal to non-zero row
          v = sm[0] % Point3(1, 0, 0);
          if (lengthSq(v) < 0.1f)
            v = normalize(sm[0] % Point3(0, 1, 0));
          else
            v = normalize(v);
        }
        else
        {
          v = sm[0] % sm[2];

          if (lengthSq(v) <= EPS)
          {
            // all rows are collinear - any vector orthogonal to non-zero row
            v = sm[0] % Point3(1, 0, 0);
            if (lengthSq(v) < 0.1f)
              v = normalize(sm[0] % Point3(0, 1, 0));
            else
              v = normalize(v);
          }
          else
            v = normalize(v);
        }
      }
      else
        v = normalize(v);
    }

    dmj.setcol(i, v);
  }

  // orthogonalize dmj
  int mi = 0;
  float mv = lengthSq(dmj.getcol(1) % dmj.getcol(2));

  for (int i = 1; i < 3; ++i)
  {
    float v = lengthSq(dmj.getcol((i + 1) % 3) % dmj.getcol((i + 2) % 3));
    if (v < mv)
    {
      mv = v;
      mi = i;
    }
  }

  if (mv < 0.9f)
  {
    Point3 a = dmj.getcol(mi);
    Point3 v = a % Point3(1, 0, 0);
    if (lengthSq(v) < 0.1f)
      v = normalize(a % Point3(0, 1, 0));
    else
      v = normalize(v);
    dmj.setcol((mi + 1) % 3, v);
    dmj.setcol((mi + 2) % 3, a % v);
  }

  if (dmj.det() < 0)
    dmj.setcol(2, -dmj.getcol(2));

  // set body tm and props
  bodyMass = mass;
  bodyMomj = dmomj;

  matrix.setcol(0, dmj.getcol(0));
  matrix.setcol(1, dmj.getcol(1));
  matrix.setcol(2, dmj.getcol(2));
  matrix.setcol(3, point3(centerOfMass));
}
