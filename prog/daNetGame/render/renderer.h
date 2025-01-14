// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_resId.h>
#include <3d/dag_resPtr.h>
#include <util/dag_oaHashNameMap.h>
#include <render/dynamicShadowRenderExtensions.h>
#include <render/world/dynamicShadowRenderExtender.h>

class BaseStreamingSceneHolder;
class RenderScene;
class LandMeshManager;
class FFTWater;
class DataBlock;
class DaSkies;
class Point3;
class Point4;
class BBox3;
class TMatrix;
class IPoint2;
class TextureIDPair;
struct SpotLight;
struct OmniLight;
typedef uint32_t light_id;
struct Driver3dPerspective;
class DPoint3;
class BloomPS;
struct CameraSetup;

namespace webui
{
struct HttpPlugin;
}

namespace scene
{
class TiledScene;
}

namespace eastl
{
template <typename T>
struct default_delete;
template <typename T, typename Deleter>
class unique_ptr;
} // namespace eastl

namespace ecs
{
class Object;
}

enum class ShadowsQuality : int;

class IRenderWorld
{
public:
  virtual void update(float dt, float real_dt, const TMatrix &itm) = 0; // can be called independent on draw
  virtual void beforeRender(float scaled_dt,
    float act_dt,
    float real_dt,
    float game_time,
    const TMatrix &view_itm,
    const DPoint3 &view_pos,
    const Driver3dPerspective &persp) = 0;
  virtual void draw(float real_dt) = 0;
  virtual void debugDraw() = 0;
  virtual void beforeLoadLevel(const DataBlock &level_blk, ecs::EntityId level_eid) = 0; // to be called from main thread

  // to be called from main thread. It will keep reference on LandMeshManager *lmesh and OWN BaseStreamingSceneHolder *scn
  virtual void onLevelLoaded(const DataBlock &level_blk) = 0;
  // to be called from loading thread
  virtual void onSceneLoaded(BaseStreamingSceneHolder *scn) = 0;
  // to be called from loading thread.
  virtual void onLightmapSet(TEXTUREID lmap_tid) = 0;
  // to be called from loading thread.
  virtual void onLandmeshLoaded(const DataBlock &level_blk, const char *bin, LandMeshManager *lmesh) = 0;

  virtual void unloadLevel() = 0;
  virtual void onSettingsChanged(const FastNameMap &changed_fields, bool apply_after_reset) = 0;
  virtual void beforeDeviceReset(bool full_reset) = 0;
  virtual void afterDeviceReset(bool full_reset) = 0;
  virtual void windowResized() = 0;
  virtual void applySettingsChanged() = 0;
  virtual void reloadCube(bool first) = 0;
  virtual void setResolution() = 0;
  virtual void getRenderingResolution(int &w, int &h) const = 0;
  virtual void overrideResolution(IPoint2 res) = 0;
  virtual void resetResolutionOverride() = 0;
  virtual void getPostFxInternalResolution(int &w, int &h) const = 0;
  virtual void getDisplayResolution(int &w, int &h) const = 0;
  virtual void updateFinalTargetFrame() = 0;
  virtual void preloadLevelTextures() = 0;
  virtual ShadowsQuality getSettingsShadowsQuality() const = 0;

  // water
  virtual void setWater(FFTWater *water) = 0; // to be called from main thread. It will keep reference on water!
  virtual FFTWater *getWater() = 0;
  virtual void setMaxWaterTessellation(int value) = 0;

  // screenshots
  virtual const ManagedTex &getFinalTargetTex() const = 0;
  virtual Texture *getFinalTargetTex2D() const = 0;
  virtual ManagedTexView getSuperResolutionScreenshot() const = 0;
  // virtual TEXTUREID getUiBlurTexId() const = 0;

  virtual bool getBoxAround(const Point3 &position, TMatrix &box) const = 0;

  virtual void setupShore(bool enabled, int texture_size, float hmap_size, float rivers_width, float significant_wave_threshold) = 0;
  virtual void setupShoreSurf(float wave_height_to_amplitude,
    float amplitude_to_length,
    float parallelism_to_wind,
    float width_k,
    const Point4 &waves_dist,
    float depth_min,
    float depth_fade_interval,
    float gerstner_speed) = 0;

  virtual dynamic_shadow_render::QualityParams getShadowRenderQualityParams() const = 0;
  virtual DynamicShadowRenderExtender::Handle registerShadowRenderExtension(DynamicShadowRenderExtender::Extension &&extension) = 0;

  virtual void shadowsInvalidate(const BBox3 &box) = 0;
  virtual void shadowsAddInvalidBBox(const BBox3 &box) = 0;
  virtual void invalidateGI(const BBox3 &model_bbox, const TMatrix &tm, const BBox3 &approx) = 0;

  virtual void setWorldBBox(const BBox3 &) = 0;

  virtual void invalidateNodeBasedResources() = 0;

  virtual bool needSeparatedUI() const = 0;

protected:
  virtual ~IRenderWorld() {}
};

void init_world_renderer(); // before create, static inits of factories

void init_renderer_per_game();
void term_renderer_per_game();

void pull_render_das();

IRenderWorld *create_world_renderer(); // Note: reentrant
void destroy_world_renderer();

void close_world_renderer(); // after destroy, static shutdowns of factories

IRenderWorld *get_world_renderer();
IRenderWorld *get_world_renderer_unsafe(); // UB if WR wasn't inited or already destroyed (i.e. can't return nullptr)

webui::HttpPlugin *get_renderer_http_plugins();
void init_fog_shader_graph_plugin();

void init_tex_streaming();
bool have_renderer(); // this is possibility of this exe to have renderer, not that it have it right now. check get_world_renderer
                      // after init if you need to know if it have it right now.

void before_draw_scene(
  int realtime_elapsed_usec, float gametime_elapsed_sec, float time_speed, ecs::EntityId cur_cam, TMatrix &view_itm);
void finish_rendering_ui();
void draw_scene(const TMatrix &view_itm);

void toggle_hide_gui();
