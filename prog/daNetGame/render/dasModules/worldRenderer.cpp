// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/dasModules/worldRenderer.h"

#include "render/rendererFeatures.h"
#include <daScript/daScript.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorDriver3d.h>
#include <render/fx/uiPostFxManager.h>
#include <dasModules/dagorTexture3d.h>
#include <rendInst/gpuObjects.h>
#include <render/daBfg/das/registerBlobType.h>


DAS_BASE_BIND_ENUM(FeatureRenderFlags,
  FeatureRenderFlags,
  VOLUME_LIGHTS,
  TILED_LIGHTS,
  POST_RESOLVE,
  COMBINED_SHADOWS,
  DEFERRED_LIGHT,
  DECALS,
  SPECULAR_CUBES,
  UPSCALE_SAMPLING_TEX,
  WAKE,
  LEVEL_SHADOWS,
  FULL_DEFERRED,
  WATER_PERLIN,
  HAZE,
  RIPPLES,
  FOM_SHADOWS,
  DOWNSAMPLED_NORMALS,
  GPU_RESIDENT_ADAPTATION,
  SSAO,
  PREV_OPAQUE_TEX,
  DOWNSAMPLED_SHADOWS,
  ADAPTATION,
  TAA,
  WATER_REFLECTION,
  BLOOM,
  MICRODETAILS,
  FORWARD_RENDERING,
  POSTFX,
  STATIC_SHADOWS,
  CLUSTERED_LIGHTS,
  LEVEL_LAND_TEXEL_SIZE,
  RENDER_FEATURES_NUM)

DAS_BASE_BIND_ENUM(
  UpdateStageInfoRender::RenderPass, RenderPass, RENDER_COLOR, RENDER_COLOR, RENDER_DEPTH, RENDER_SHADOW, RENDER_MAIN);

namespace bind_dascript
{

struct CameraParamsAnnotation : das::ManagedStructureAnnotation<CameraParams, false>
{
  CameraParamsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CameraParams", ml, "::CameraParams")
  {
    addField<DAS_BIND_MANAGED_FIELD(viewRotJitterProjTm)>("viewRotJitterProjTm");
    addField<DAS_BIND_MANAGED_FIELD(viewRotTm)>("viewRotTm");

    addField<DAS_BIND_MANAGED_FIELD(viewItm)>("viewItm");
    addField<DAS_BIND_MANAGED_FIELD(viewTm)>("viewTm");

    addField<DAS_BIND_MANAGED_FIELD(cameraWorldPos)>("cameraWorldPos");
    addField<DAS_BIND_MANAGED_FIELD(negRoundedCamPos)>("negRoundedCamPos");
    addField<DAS_BIND_MANAGED_FIELD(negRemainderCamPos)>("negRemainderCamPos");

    addField<DAS_BIND_MANAGED_FIELD(noJitterPersp)>("noJitterPersp");
    addField<DAS_BIND_MANAGED_FIELD(jitterPersp)>("jitterPersp");

    // TODO: TMatrix4_vec4 is not bound to das
    // addField<DAS_BIND_MANAGED_FIELD(noJitterProjTm)>("noJitterProjTm");
    // addField<DAS_BIND_MANAGED_FIELD(noJitterGlobtm)>("noJitterGlobtm");
    // addField<DAS_BIND_MANAGED_FIELD(jitterProjTm)>("jitterProjTm");
    // addField<DAS_BIND_MANAGED_FIELD(jitterGlobtm)>("jitterGlobtm");

    addField<DAS_BIND_MANAGED_FIELD(noJitterFrustum)>("noJitterFrustum");
    addField<DAS_BIND_MANAGED_FIELD(jitterFrustum)>("jitterFrustum");
  }
};

class WorldRendererModule final : public das::Module
{
public:
  WorldRendererModule() : das::Module("WorldRenderer")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("DagorTexture3D"));

    addAnnotation(das::make_smart<CameraParamsAnnotation>(lib));
    dabfg::das::register_external_interop_type<CameraParams>(lib);

    addEnumeration(das::make_smart<EnumerationRenderPass>());
    das::addEnumFlagOps<UpdateStageInfoRender::RenderPass>(*this, lib, "UpdateStageInfoRender::RenderPass");

    addEnumeration(das::make_smart<EnumerationFeatureRenderFlags>());
    das::addExtern<DAS_BIND_FUN(bind_dascript::toggleFeature)>(*this, lib, "toggleFeature", das::SideEffects::modifyExternal,
      "bind_dascript::toggleFeature");
    das::addExtern<DAS_BIND_FUN(UiPostFxManager::setBlurUpdateEnabled)>(*this, lib, "setBlurUpdateEnabled",
      das::SideEffects::modifyExternal, "UiPostFxManager::setBlurUpdateEnabled");
    void (&pf)(const IPoint2 &, const IPoint2 &, int) = UiPostFxManager::updateFinalBlurred;
    das::addExtern<decltype(&pf), UiPostFxManager::updateFinalBlurred>(*this, lib, "updateFinalBlurred",
      das::SideEffects::modifyExternal, "UiPostFxManager::updateFinalBlurred");
    das::addExtern<DAS_BIND_FUN(erase_grass)>(*this, lib, "erase_grass", das::SideEffects::modifyExternal, "erase_grass");
    das::addExtern<DAS_BIND_FUN(invalidate_after_heightmap_change)>(*this, lib, "invalidate_after_heightmap_change",
      das::SideEffects::modifyExternal, "invalidate_after_heightmap_change");
    das::addExtern<DAS_BIND_FUN(bind_dascript::worldRenderer_getWorldBBox3)>(*this, lib, "worldRenderer_getWorldBBox3",
      das::SideEffects::worstDefault, "bind_dascript::worldRenderer_getWorldBBox3");
    das::addExtern<DAS_BIND_FUN(bind_dascript::worldRenderer_shadowsInvalidate)>(*this, lib, "worldRenderer_shadowsInvalidate",
      das::SideEffects::modifyExternal, "bind_dascript::worldRenderer_shadowsInvalidate");
    das::addExtern<DAS_BIND_FUN(bind_dascript::worldRenderer_invalidateAllShadows)>(*this, lib, "worldRenderer_invalidateAllShadows",
      das::SideEffects::modifyExternal, "bind_dascript::worldRenderer_invalidateAllShadows");
    das::addExtern<DAS_BIND_FUN(bind_dascript::worldRenderer_renderDebug)>(*this, lib, "worldRenderer_renderDebug",
      das::SideEffects::modifyExternal, "bind_dascript::worldRenderer_renderDebug");
    das::addExtern<DAS_BIND_FUN(bind_dascript::worldRenderer_getDynamicResolutionTargetFps)>(*this, lib,
      "worldRenderer_getDynamicResolutionTargetFps", das::SideEffects::accessExternal,
      "bind_dascript::worldRenderer_getDynamicResolutionTargetFps");
    das::addExtern<DAS_BIND_FUN(bind_dascript::does_world_renderer_exist)>(*this, lib, "does_world_renderer_exist",
      das::SideEffects::accessExternal, "bind_dascript::does_world_renderer_exist");

    das::addExtern<void (*)(int &, int &), &::get_display_resolution>(*this, lib, "get_display_resolution",
      das::SideEffects::modifyArgument, "::get_display_resolution");
    das::addExtern<void (*)(int &, int &), &::get_rendering_resolution>(*this, lib, "get_rendering_resolution",
      das::SideEffects::modifyArgument, "::get_rendering_resolution");
    das::addExtern<void (*)(int &, int &), &::get_postfx_resolution>(*this, lib, "get_postfx_resolution",
      das::SideEffects::modifyArgument, "::get_postfx_resolution");

    G_UNUSED(pf);

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <render/fx/uiPostFxManager.h>\n";
    tw << "#include <render/dasModules/worldRenderer.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(WorldRendererModule, bind_dascript);
