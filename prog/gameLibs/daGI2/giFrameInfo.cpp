// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "giFrameInfo.h"
#include <render/viewVecs.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>

DaGIFrameInfo get_frame_info(const DPoint3 &world_pos, const TMatrix &viewItm, const TMatrix4 &proj, float zn, float zf)
{
  DaGIFrameInfo fi;

  fi.projTm = proj;
  fi.viewItm = viewItm;
  fi.viewTm = orthonormalized_inverse(viewItm);
  TMatrix viewRot = fi.viewTm;
  viewRot.setcol(3, 0.0f, 0.0f, 0.0f);
  d3d::calcglobtm(fi.viewTm, fi.projTm, fi.globTm);
  d3d::calcglobtm(viewRot, fi.projTm, fi.globTmNoOfs);
  fi.znear = zn;
  fi.zfar = zf;
  G_ASSERT(fi.zfar > 0 && fi.znear > 0);
  fi.viewZ = viewItm.getcol(2);
  fi.world_view_pos = world_pos;
  ViewVecs v = calc_view_vecs(fi.viewTm, fi.projTm);
  fi.viewVecLT = v.viewVecLT;
  fi.viewVecRT = v.viewVecRT;
  fi.viewVecLB = v.viewVecLB;
  fi.viewVecRB = v.viewVecRB;
  return fi;
}