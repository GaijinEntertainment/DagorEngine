//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <ioSys/dag_dataBlock.h>
#include <EditorCore/ec_interface.h>
#include <de3_interface.h>

// multipoint placement

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

  void makePointsFromBox(const BBox3 &box, float inward_ofs = 0.05, bool box_min_y = false)
  {
    Point3 inb = box.width() * inward_ofs;
    float y0 = box_min_y ? box[0].y : 0;
    if (inb.x >= inb.z)
    {
      if (mpOrientType == MP_ORIENT_3POINT)
        mpPoint1.set(box[1].x - inb.x, y0, (box[0].z + box[1].z) / 2);
      else
      {
        mpPoint1.set(box[1].x - inb.x, y0, box[1].z - inb.z);
        mpPoint4.set(box[1].x - inb.x, y0, box[0].z + inb.z);
      }
      mpPoint2.set(box[0].x + inb.x, y0, box[0].z + inb.z);
      mpPoint3.set(box[0].x + inb.x, y0, box[1].z - inb.z);
    }
    else
    {
      if (mpOrientType == MP_ORIENT_3POINT)
        mpPoint1.set((box[0].x + box[1].x) / 2, y0, box[1].z - inb.z);
      else
      {
        mpPoint1.set(box[0].x + inb.x, y0, box[1].z - inb.z);
        mpPoint4.set(box[1].x - inb.x, y0, box[1].z - inb.z);
      }
      mpPoint2.set(box[0].x + inb.x, y0, box[0].z + inb.z);
      mpPoint3.set(box[1].x - inb.x, y0, box[0].z + inb.z);
    }
  }
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

static inline bool load_multipoint_data(MpPlacementRec &mpRec, const DataBlock &objBlk, const BBox3 &objBbox)
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

  if (objBlk.getBool("mpUseBbox", !objBbox.isempty()))
  {
    if (objBbox.isempty())
    {
      DAEDITOR3.conError("mpUseBbox:b=yes is not allowed for empty bbox (landclass case)");
      return false;
    }
    mpRec.makePointsFromBox(objBbox, objBlk.getReal("mpInwardBoxOfs", 0.05), objBlk.getBool("mpUseBboxMinY", false));
  }
  else
  {
    mpRec.mpPoint1.set(0, 0, 0.5);
    mpRec.mpPoint2.set(0.5, 0, 0);
    mpRec.mpPoint3.set(-0.4, 0, -0.4);
  }

  mpRec.mpPoint1 = objBlk.getPoint3("mpPoint1", mpRec.mpPoint1);
  mpRec.mpPoint2 = objBlk.getPoint3("mpPoint2", mpRec.mpPoint2);
  mpRec.mpPoint3 = objBlk.getPoint3("mpPoint3", mpRec.mpPoint3);

  mpRec.computePivotBc();
  return true;
}
