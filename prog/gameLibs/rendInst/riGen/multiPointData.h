#pragma once

#include <math/dag_Point3.h>
#include <ioSys/dag_roDataBlock.h>
#include <startup/dag_globalSettings.h>


// multipoint placement

namespace rendinstgen
{
struct MpPlacementRec
{
  short mpOrientType = MP_ORIENT_NONE;
  Point3 mpPoint1 = ZERO<Point3>();
  Point3 mpPoint2 = ZERO<Point3>();
  Point3 mpPoint3 = ZERO<Point3>();
  Point3 mpPoint4 = ZERO<Point3>();
  float pivotBc31 = 0, pivotBc21 = 0;

  enum
  {
    MP_ORIENT_NONE,
    MP_ORIENT_YZ,
    MP_ORIENT_XY,
    MP_ORIENT_3POINT,
    MP_ORIENT_QUAD
  };

  void computePivotBc()
  {
    Point3 u = mpPoint2 - mpPoint1;
    Point3 v = mpPoint3 - mpPoint1;
    Point3 w = -mpPoint1;
    float uv = (Point3::x0z(u) % Point3::x0z(v)).y;
    float vw = (Point3::x0z(v) % Point3::x0z(w)).y;
    float uw = (Point3::x0z(u) % Point3::x0z(w)).y;
    pivotBc21 = safediv(vw, -uv);
    pivotBc31 = safediv(uw, uv);
  }
};

static inline bool load_multipoint_data(MpPlacementRec &mpRec, const RoDataBlock &objBlk)
{
  const char *mp_placement_str = objBlk.getStr("placement", "none");
  if (stricmp(mp_placement_str, "yz") == 0)
    mpRec.mpOrientType = MpPlacementRec::MP_ORIENT_YZ;
  else if (stricmp(mp_placement_str, "xy") == 0)
    mpRec.mpOrientType = MpPlacementRec::MP_ORIENT_XY;
  else if (stricmp(mp_placement_str, "3point") == 0)
    mpRec.mpOrientType = MpPlacementRec::MP_ORIENT_3POINT;
  else if (stricmp(mp_placement_str, "none") == 0)
    mpRec.mpOrientType = MpPlacementRec::MP_ORIENT_NONE;
  else
  {
    mpRec.mpOrientType = MpPlacementRec::MP_ORIENT_NONE;
    return false;
  }

  if (objBlk.getBool("mpUseBbox", false))
  {
    char buf[1 << 10];
    logerr("mpUseBbox:b=yes is not allowed for landclass%s", dgs_get_fatal_context(buf, sizeof(buf)));
  }
  mpRec.mpPoint1.set(0, 0, 0.5);
  mpRec.mpPoint2.set(0.5, 0, 0);
  mpRec.mpPoint3.set(-0.4, 0, -0.4);

  mpRec.mpPoint1 = objBlk.getPoint3("mpPoint1", mpRec.mpPoint1);
  mpRec.mpPoint2 = objBlk.getPoint3("mpPoint2", mpRec.mpPoint2);
  mpRec.mpPoint3 = objBlk.getPoint3("mpPoint3", mpRec.mpPoint3);

  mpRec.computePivotBc();
  return true;
}
} // namespace rendinstgen
