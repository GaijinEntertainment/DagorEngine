//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>

struct ComputedShadowSplit
{
protected:
  // Unit Cube(UC) <-> Normalized Device Coordinates(NDC) transformations
  static TMatrix4 NDCtoUC;
  static TMatrix4 UCtoNDC;

public:
  TMatrix4 proj;       // project from world space into split unit cube at
                       // post projection light space
                       // note: this transformation include at unit cube
                       // ONLY FRUSTUM POINTS without all potential casters
  TMatrix4 visTM;      // transformation(without warping) into light space unit cube
                       // for visibility test
  TMatrix4 texTM;      // from UC to texture space
  BBox3 splitBoxWS;    // box of split in World Space
  BBox3 splitBoxLocal; // box of split in visTM

  float splitZBias; // correct ZBias(at post projection space) computed for this split

  inline TMatrix4 getProj() const { return proj * UCtoNDC; }
  inline TMatrix4 getTexTM() const { return proj * texTM; }

  static TMatrix4 getNDCtoUC() { return NDCtoUC; }
  static TMatrix4 getUCtoNDC() { return UCtoNDC; }
  bool visible(const Point3 &c, float rad) const { return BSphere3(c, rad) & splitBoxWS; }
  bool visible(const BBox3 &box) const
  {
    if (!(box & splitBoxWS))
      return false;

    BBox3 visBox;

    for (int m = 0; m < 8; ++m)
    {
      Point3 p = box.point(m) * visTM;
      visBox += p;
    }

    static const BBox3 unitCube(Point3(0.0f, 0.0f, 0.0f), Point3(1.0f, 1.0f, 1.0f));
    if (visBox & unitCube)
      return true;
    return false;
  }
};
