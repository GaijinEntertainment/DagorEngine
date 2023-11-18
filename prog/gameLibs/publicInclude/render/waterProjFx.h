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
#include <util/dag_simpleString.h>

class TextureIDPair;

struct IWwaterProjFxRenderHelper
{
  virtual bool render_geometry() = 0;
  virtual bool render_geometry_without_aa() { return false; }
  virtual void prepare_taa_reprojection_blend(const TEXTUREID *prev_frame_targets, const TEXTUREID *cur_frame_targets)
  {
    G_UNUSED(prev_frame_targets);
    G_UNUSED(cur_frame_targets);
  };
};


class WaterProjectedFx
{
public:
  enum
  {
    MAX_TARGETS = 4
  };

  struct TargetDesc
  {
    uint32_t texFmt;
    SimpleString texName;
    E3DCOLOR clearColor;
  };

  WaterProjectedFx(int frame_width, int frame_height, dag::Span<TargetDesc> target_descs,
    const char *taa_reprojection_blend_shader_name, bool own_textures = true);

  void setTextures();
  void setView();
  bool getView(TMatrix4 &view_tm, TMatrix4 &proj_tm, Point3 &camera_pos);
  bool isValidView() const;

  void prepare(const TMatrix &view_tm, const TMatrix &view_itm, const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, float water_level,
    float significant_wave_height, int frame_no, bool change_projection);
  bool render(IWwaterProjFxRenderHelper *render_helper);
  bool render(IWwaterProjFxRenderHelper *render_helper, dag::Span<const TextureIDPair> targets,
    dag::Span<const TextureIDPair> taaTemp0 = {}, dag::Span<const TextureIDPair> taaTemp1 = {});
  void clear(bool forceClear = false);

  // It's assumed that the targets won't be modified outside of this class if they're owned by this class.
  Texture *getTarget(int target_id)
  {
    G_ASSERT(ownTextures && target_id < nTargets);
    return internalTargets[target_id].getTex2D();
  }
  uint32_t getTargetAdditionalFlags() const;

private:
  void setView(const TMatrix &view_tm, const TMatrix4 &proj_tm);
  void setWaterMatrix(const TMatrix4 &glob_tm);

  TMatrix4 newProjTM, newProjTMJittered;
  TMatrix newViewTM, newViewItm;
  Point3 savedCamPos;
  Point2 curJitter;
  float waterLevel;
  int numIntersections;

  TMatrix4 prevGlobTm;

  int frameWidth;
  int frameHeight;
  int nTargets;
  UniqueTexHolder internalTargets[MAX_TARGETS];
  E3DCOLOR targetClearColors[MAX_TARGETS];
  const bool ownTextures;
  bool targetsCleared;

  bool taaEnabled;
  RTargetPool::Ptr taaRtTempPools[MAX_TARGETS];
  PostFxRenderer taaRenderer;

  int globalFrameId;
};
