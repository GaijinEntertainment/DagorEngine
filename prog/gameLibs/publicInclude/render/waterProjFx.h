//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_resourcePool.h>
#include <math/dag_TMatrix4.h>
#include <generic/dag_carray.h>
#include <shaders/dag_postFxRenderer.h>

class DataBlock;

struct IWwaterProjFxRenderHelper
{
  virtual bool render_geometry() = 0;
  virtual void render_geometry_without_aa() {}
  virtual void finish_rendering() {}
};


class WaterProjectedFx
{
public:
  WaterProjectedFx(int tex_width, int tex_height, bool temporal_repojection = true);

  void setTexture();
  void setView();
  bool getView(TMatrix4 &view_tm, TMatrix4 &proj_tm, Point3 &camera_pos);
  bool isValidView() const;

  void prepare(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, float water_level,
    float significant_wave_height, int frame_no);
  void render(IWwaterProjFxRenderHelper *render_helper);
  void clear();

private:
  void setView(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix &view_itm);
  void setWaterMatrix(const TMatrix4 &glob_tm);

  TMatrix4 newProjTM, newProjTMJittered;
  TMatrix newViewTM, newViewItm;
  Point3 savedCamPos;
  Point2 curJitter;
  float waterLevel;
  int numIntersections;

  TMatrix4 prevGlobTm;

  UniqueTexHolder waterProjectionFxTex;
  RTargetPool::Ptr rtTemp;
  PostFxRenderer combinedWaterFoam;
  int waterProjectionFxTexWidth, waterProjectionFxTexHeight;
  int globalFrameId;
  bool temporalRepojection;
};
