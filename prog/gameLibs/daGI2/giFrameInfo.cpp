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

  // fixme: pass jitter
  fi.projTmUnjittered = fi.projTm;
  fi.globTmUnjittered = fi.globTm;
  fi.globTmNoOfsUnjittered = fi.globTmNoOfs;

  return fi;
}

TMatrix4 get_reprojection_campos_to_unjittered_history(const DaGIFrameInfo &current, const DaGIFrameInfo &prev)
{
  const DPoint3 move = current.world_view_pos - prev.world_view_pos;
  TMatrix4 prevGlobTm = prev.globTmNoOfsUnjittered;

  double translate[4] = {prevGlobTm[0][0] * move.x + prevGlobTm[1][0] * move.y + prevGlobTm[2][0] * move.z + prevGlobTm[3][0],
    prevGlobTm[0][1] * move.x + prevGlobTm[1][1] * move.y + prevGlobTm[2][1] * move.z + prevGlobTm[3][1],
    prevGlobTm[0][2] * move.x + prevGlobTm[1][2] * move.y + prevGlobTm[2][2] * move.z + prevGlobTm[3][2],
    prevGlobTm[0][3] * move.x + prevGlobTm[1][3] * move.y + prevGlobTm[2][3] * move.z + prevGlobTm[3][3]};

  prevGlobTm.setrow(3, translate[0], translate[1], translate[2], translate[3]);
  return prevGlobTm;
}
