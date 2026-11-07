//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_tex3d.h>
#include <vecmath/dag_vecMathDecl.h>
#include <EASTL/functional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>

#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <render/deferredRT.h>


class DeferredRenderTarget;
class TMatrix4;
struct Frustum;
struct RiGenVisibility;
struct CameraParams;


class SatelliteRenderer
{
public:
  using LandmeshHeightGetterCb = eastl::function<bool(const Point2 &pos2d, float &height)>;
  using RenderWaterCb = eastl::function<void(Texture *color_target, const CameraParams &camera, Texture *depth)>;
  using RenderLandmeshCb =
    eastl::function<void(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, const Point3 &view_pos)>;
  using ClipmapGetLastUpdatedTileCountCb = eastl::function<int()>;
  using TexStreamingGetPendingCountCb = eastl::function<int()>;
  using SaveTileCb = eastl::function<void(const ManagedTex &tex, const char *quadkey)>;
  using SetSceneCameraPosCb = eastl::function<void(const Point3 &pos)>;
  using ScanDoneCb = eastl::function<void(bool completed)>;

  struct CallbackParams
  {
    LandmeshHeightGetterCb landmeshHeightGetter;
    RenderWaterCb renderWater;
    RenderLandmeshCb renderLandmesh;
    ClipmapGetLastUpdatedTileCountCb clipmapGetLastUpdatedTileCount;
    TexStreamingGetPendingCountCb texStreamingGetPendingCount;
    SaveTileCb saveTile;
    SetSceneCameraPosCb setSceneCameraPos;
    ScanDoneCb onScanDone;
  };

  struct TargetParams
  {
    eastl::string resolveShaderName;
    eastl::fixed_vector<unsigned, DeferredRT::MAX_NUM_MRT> gbufFormats;
    unsigned depthFormat = TEXFMT_DEPTH32;
    unsigned targetFormat = TEXFMT_DEFAULT;
  };

  // tiled map generation; bbox must be square
  struct ScanParams
  {
    Point2 leftTop = Point2(-2048, -2048);
    Point2 rightBottom = Point2(2048, 2048);
    int tileWidth = 1024;      // tile size in pixels, clamped to 64..4096
    int zlevels = 5;           // 1 << zlevels tiles per side, clamped to 1..10
    float fixedHeight = -1.f;  // > 0: fixed camera height; <= 0: land height + render_sat.height_ofs
    float heightOffset = -1.f; // > 0 overrides render_sat.height_ofs; <= 0 keeps the convar value
  };
  // sets render_sat.* convars, so console edits still apply during the scan; always starts from the first tile;
  // a rect that is empty or not square (the sides differ by more than 1 cm) logs an error, changes nothing and returns false;
  // init() first: renderScripted() does nothing until init() sets the callbacks
  bool startScan(const ScanParams &params);
  // stops only the scan and resets its progress; render_sat.draw_every_frame stays on and keeps isActive() true
  void stopScan();
  bool isScanning() const;

  SatelliteRenderer();
  ~SatelliteRenderer();

  void init(const TargetParams &target_params, const CallbackParams &callback_params);
  bool isActive() const;
  // does nothing before init(); reads only camera_params.viewItm and restores camera_params on return;
  // renderWater gets the satellite viewTm, frustums and persps, viewItm at the satellite position, and the caller
  // values in the other fields; restores the four view vecs, globtm_inv and downsampled_far_depth_tex
  void renderScripted(CameraParams &camera_params);
  void cleanupResources();

private:
  void ensureTargets();
  void renderFromPos(Point3 in_pos, const CallbackParams &callback_params, CameraParams &camera_params);
  void adjustRenderHeight(Point3 &in_pos, const CallbackParams &callback_params);
  void scanSettleCheck(int vtex_updates, int tex_streaming_pending);

  TargetParams targetParams;
  CallbackParams callbackParams;

  eastl::unique_ptr<DeferredRenderTarget> renderTargetGbuf;
  UniqueTex targetTex;
  RiGenVisibility *riGenVisibility = nullptr;

  IPoint2 scanIndex = IPoint2::ZERO;
  int scanSettleFrameId = 0;
};
