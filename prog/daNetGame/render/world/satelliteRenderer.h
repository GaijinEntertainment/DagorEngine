// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/functional.h>

#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>


class DeferredRenderTarget;
struct RiGenVisibility;
struct CameraParams;


class SatelliteRenderer
{
public:
  using LandmeshHeightGetterCb = eastl::function<bool(const Point2 &pos2d, float &height)>;
  using RenderWaterCb = eastl::function<void(Texture *color_target, const TMatrix &itm, Texture *depth)>;
  using RenderLandmeshCb =
    eastl::function<void(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, const Point3 &view_pos)>;
  using ClipmapGetLastUpdatedTileCountCb = eastl::function<int()>;

  struct CallbackParams
  {
    LandmeshHeightGetterCb landmeshHeightGetter;
    RenderWaterCb renderWater;
    RenderLandmeshCb renderLandmesh;
    ClipmapGetLastUpdatedTileCountCb clipmapGetLastUpdatedTileCount;
  };

  using GetParamsCb = eastl::function<CallbackParams()>;

  void initParamsGetter(const GetParamsCb &get_params_cb) { getParamsCb = get_params_cb; }
  void renderScripted(CameraParams &camera_params);
  void cleanupResources();

private:
  void ensureTargets();
  void renderFromPos(Point3 in_pos, const CallbackParams &callback_params, CameraParams &camera_params);
  void adjustRenderHeight(Point3 &in_pos, const CallbackParams &callback_params);
  void scanSettleCheck(int vtex_updates);

  GetParamsCb getParamsCb;

  eastl::unique_ptr<DeferredRenderTarget> renderTargetGbuf;
  UniqueTexHolder targetTex;
  RiGenVisibility *riGenVisibility = nullptr;

  IPoint2 scanIndex = IPoint2::ZERO;
  int scanSettleFrameId = 0;
};
