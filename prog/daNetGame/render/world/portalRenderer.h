// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/functional.h>
#include <EASTL/array.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>
#include <shaders/dag_postFxRenderer.h>
#include <rendInst/riExtraRenderer.h>
#include <3d/dag_texStreamingContext.h>

#include <daECS/core/entityComponent.h>


class DeferredRenderTarget;
struct RiGenVisibility;
struct CameraParams;

struct PortalParams
{
  float portalRange = 200;
  bool isBidirectional = false;
  bool useDenseFog = false;
  float fogStartDist = 10;
};

class PortalRenderer
{
public:
  using RenderWaterCb = eastl::function<void(Texture *color_target, const TMatrix &itm, Texture *depth)>;
  using RenderLandmeshCb =
    eastl::function<void(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, const Point3 &view_pos)>;
  using RenderSkyCb = eastl::function<void(const TMatrix &view, const TMatrix4 &proj, const Driver3dPerspective &persp)>;
  using PrepareClipmapCb = eastl::function<void(const Point3 &)>;
  using RenderRiCb = eastl::function<void(const TMatrix &view,
    const RiGenVisibility &visibility,
    const TexStreamingContext &tex_ctx,
    const rendinst::render::RiExtraRenderer *riex_renderer)>;

  struct CallbackParams
  {
    RenderWaterCb renderWater;
    RenderLandmeshCb renderLandmesh;
    PrepareClipmapCb prepareClipmap;
    RenderRiCb renderRiPrepass;
    RenderRiCb renderRiNormal;
    RenderRiCb renderRiTrans;
  };

  using GetParamsCb = eastl::function<CallbackParams()>;

  PortalRenderer();
  ~PortalRenderer();

  void initCallbacks(const GetParamsCb &get_params_cb)
  {
    getParamsCb = get_params_cb;
    skipFrameCount = 0;
  }
  void update(float dt);
  void render(CameraParams &camera_params);


  void updatePortal(int portal_index, const TMatrix &source_tm, const TMatrix &target_tm, const PortalParams &params);
  int allocatePortal();
  void freePortal(int portal_index);
  int getActivePortalCubeSlot(int portal_index) const;
  int getRenderedPortalCubeSlot(int portal_index, TMatrix &portal_tm, float &life, bool &is_bidirectional) const;

private:
  static constexpr int MAX_RENDERED_CUBES = 4;
  static constexpr int SKIP_FRAME_COUNT = 100;

  class PortalVisibility
  {
    RiGenVisibility *riGenVisibility;

  public:
    void init(int ri_min_lod);
    void close();

    PortalVisibility() { riGenVisibility = nullptr; }
    ~PortalVisibility() { close(); }

    RiGenVisibility *getVisibility() const;

    void startVisibilityJob(const mat44f &cull_tm, const Point3 &view_pos);
  };

  void renderCube(int portal_cube_index, CameraParams &camera_params);
  void copyFrame(int cube_index, int cube_face);
  void copyFrameImpl(int cube_index, int face_start, int face_count, int from_mip, int mips_count);

  bool checkPortalIndex(int portal_index) const;

  float calcDistanceToPortal(int portal_index, const CameraParams &camera_params) const;
  void getFurthestActivePortal(int best_inactive_portal_index, const CameraParams &camera_params, int &cube_slot, float &max_distance);
  void getClosestInactivePortal(const CameraParams &camera_params, int &portal_index, float &min_distance);
  void updateActivePortalData(const CameraParams &camera_params);
  int getClosestValidPortalCubeSlot(const CameraParams &camera_params) const;
  void startCubeRendering(int cube_slot, bool is_same_portal_index);

  GetParamsCb getParamsCb;

  eastl::unique_ptr<DeferredRenderTarget> renderTargetGbuf;

  PortalVisibility portalVisibility;

  UniqueTexHolder envCubeTexArr;
  UniqueTex tmpRenderTargetTex;

  PostFxRenderer copyTargetRenderer;

  enum class PortalState
  {
    Inactive,
    Dirty,
    IsBaking,
    Rendered,
    Deleted
  };

  struct PortalRenderingData
  {
    TMatrix transform = TMatrix::IDENT;
    Point3 sourcePos;
    PortalParams params;
  };

  struct PortalData
  {
    PortalState state = PortalState::Inactive;
    PortalRenderingData renderingData;
  };

  eastl::vector<PortalData> portalDataVec;
  eastl::array<int, MAX_RENDERED_CUBES> activePortalIndices;
  eastl::array<float, MAX_RENDERED_CUBES> activePortalLifeArr;

  enum RenderStage
  {
    Start = 0,
    WarmupRi = 1,
    ClipmapPrepared = 6 + 1, // from ri_invalid_frame_cnt (and also need to account for texture streaming) TODO: make it more flexible
    Done
  };

  class BakingData
  {
    int activeRenderedCubeIndex = -1;
    PortalRenderingData cachedRenderingData;
    int activeFace;
    RenderStage activeStage;
    bool hasValidPreviousRenderedState;

  public:
    BakingData() { resetBaking(); }

    void startBaking(int cube_index, const PortalRenderingData &data, bool is_same_portal_index)
    {
      G_ASSERT_RETURN(cube_index >= 0 && cube_index < MAX_RENDERED_CUBES, );

      activeRenderedCubeIndex = cube_index;
      cachedRenderingData = data;
      activeStage = RenderStage::Start;
      activeFace = 0;

      hasValidPreviousRenderedState = is_same_portal_index;
    }

    void resetBaking()
    {
      activeRenderedCubeIndex = -1;
      activeStage = RenderStage::Start;
      activeFace = 0;
      hasValidPreviousRenderedState = false;
    }

    int getCubeIndex() const
    {
      G_ASSERTF_ONCE(isBaking(), "Portal baking data is not ready!");
      return activeRenderedCubeIndex;
    }

    PortalRenderingData getCachedData() const
    {
      G_ASSERTF_ONCE(isBaking(), "Portal baking data is not ready!");
      return cachedRenderingData;
    }

    RenderStage getStage() const
    {
      G_ASSERTF_ONCE(isBaking(), "Portal baking data is not ready!");
      return activeStage;
    }

    void advanceBakingStage()
    {
      G_ASSERTF_ONCE(isBaking(), "Portal baking data is not ready!");
      activeStage = (RenderStage)(min((int)activeStage + 1, (int)RenderStage::Done));
      if (activeStage >= RenderStage::Done && activeFace < 5)
      {
        activeStage = RenderStage::Start;
        activeFace = min(activeFace + 1, 5);
      }
    }

    int getFace() const
    {
      G_ASSERTF_ONCE(isBaking(), "Portal baking data is not ready!");
      return activeFace;
    }

    bool isFinished() const
    {
      G_ASSERTF_ONCE(isBaking(), "Portal baking data is not ready!");
      return activeFace >= 5 && activeStage >= RenderStage::Done;
    }

    bool isCubeBakingNew(int cube_index) const
    {
      return isBaking() && activeRenderedCubeIndex == cube_index && !hasValidPreviousRenderedState;
    }

    bool isBaking() const { return activeRenderedCubeIndex != -1; }
  };

  BakingData bakingData;

  SharedTexHolder shatteredGlassMaskTex;

  int skipFrameCount = 0;
};

ECS_DECLARE_BOXED_TYPE(PortalRenderer);

namespace portal_renderer_mgr
{
PortalRenderer *query_portal_renderer();

void update_portal(int portal_index, const TMatrix &source_tm, const TMatrix &target_tm, const PortalParams &params);
int allocate_portal();
void free_portal(int portal_index);
int get_rendered_portal_cube_index(int portal_index, TMatrix &portal_tm, float &life, bool &is_bidirectional);
} // namespace portal_renderer_mgr
