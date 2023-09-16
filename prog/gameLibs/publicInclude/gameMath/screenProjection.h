//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>

inline bool project_world_to_screen(const float width, const float height, const float zn, const TMatrix4 &globtm,
  const Point3 &world_pos, const float screen_treshold, Point2 &out_screen_pos, float &out_depth)
{
  bool res = true;
  Point4 pos = Point4::xyz1(world_pos);
  pos = pos * globtm;
  out_depth = safediv(pos.z, pos.w);

  if (pos.z >= pos.w) // Check only nearplane, allow world_pos to be beyond the farplane.
  {
    res = false;
    out_screen_pos.x = -10000;
    out_screen_pos.y = -10000;

    if (fabsf(pos.w) > 0.0001)
    {
      pos *= safediv(sign(pos.z + zn), pos.w);
      out_screen_pos.x = width * (0.5f * pos.x + 0.5f);
      out_screen_pos.y = height * (-0.5f * pos.y + 0.5f);
    }
  }
  else
  {
    pos *= safediv(1.0f, pos.w);
    out_screen_pos.x = width * (0.5f * pos.x + 0.5f);
    out_screen_pos.y = height * (-0.5f * pos.y + 0.5f);
  }

  if (out_screen_pos.y < -screen_treshold || out_screen_pos.y >= height + screen_treshold || out_screen_pos.x < -screen_treshold ||
      out_screen_pos.x >= width + screen_treshold)
    res = false;

  return res;
}

inline TMatrix make_ui3d_local_tm(const TMatrix &local_tm, const float depth, const float width, const float height,
  const float persp_wk, const float persp_hk, const int align = 0, const int valign = 0)
{
  TMatrix wtm = local_tm;
  wtm.setcol(0, local_tm.getcol(0) / width * max(depth / persp_wk, 1.0f) * 2.0f);
  wtm.setcol(1, local_tm.getcol(1) / height * max(depth / persp_hk, 1.0f) * 2.0f);
  wtm.setcol(3, local_tm.getcol(3) + wtm.getcol(0) * (align * width / 2) + wtm.getcol(1) * (valign * height / 2));
  return wtm;
}

inline TMatrix make_ui3d_local_tm(const float depth, const float width, const float height, const float persp_wk, const float persp_hk)
{
  TMatrix localTm = TMatrix::IDENT;
  localTm.setcol(1, Point3(0, -1, 0));
  localTm.setcol(3, Point3(0, 0, depth));
  return make_ui3d_local_tm(localTm, depth, width, height, persp_wk, persp_hk, -1, -1);
}

inline TMatrix make_ui3d_local_tm(const Point3 &pos, const Point3 &dir, const float depth, const float width, const float height,
  const float persp_wk, const float persp_hk)
{
  Point3 fwd, up, right;
  ortho_normalize(dir, Point3(0, -1, 0), fwd, up, right);
  TMatrix localTm;
  localTm.setcol(0, right);
  localTm.setcol(1, up);
  localTm.setcol(2, fwd);
  localTm.setcol(3, pos);
  return make_ui3d_local_tm(localTm, depth, width, height, persp_wk, persp_hk, 0, 0);
}
