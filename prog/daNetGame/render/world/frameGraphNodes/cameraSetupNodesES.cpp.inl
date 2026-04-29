// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/daFrameGraph/daFG.h>
#include <render/motionVectorAccess.h>

#include <math/dag_TMatrix4D.h>

#include <main/water.h>
#include <render/antiAliasing_legacy.h>
#include <render/renderEvent.h>
#include <render/skies.h>
#include <render/subFrameSample.h>
#include <render/volumetricLights/volumetricLights.h>
#include <render/world/antiAliasingMode.h>
#include <render/world/cameraParams.h>
#include <render/world/depthBounds.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/reprojectionTm.h>
#include <render/world/sunParams.h>
#include <render/world/waterRenderMode.h>
#include <render/world/wrDispatcher.h>

#include <EASTL/shared_ptr.h>


template <typename T>
using BlobHandle = dafg::VirtualResourceHandle<T, false, false>;

namespace var
{
ShaderVariableInfo depth_bounds_support("depth_bounds_support", true);
ShaderVariableInfo dissolve_frame_counter("dissolve_frame_counter", true);
ShaderVariableInfo uv_temporal_jitter("uv_temporal_jitter", true);
ShaderVariableInfo gi_hero_cockpit_distance("gi_hero_cockpit_distance", true);
ShaderVariableInfo prev_hero_matrixX("prev_hero_matrixX", true);
ShaderVariableInfo prev_hero_matrixY("prev_hero_matrixY", true);
ShaderVariableInfo prev_hero_matrixZ("prev_hero_matrixZ", true);
ShaderVariableInfo hero_matrixX("hero_matrixX", true);
ShaderVariableInfo hero_matrixY("hero_matrixY", true);
ShaderVariableInfo hero_matrixZ("hero_matrixZ", true);
ShaderVariableInfo hero_is_cockpit("hero_is_cockpit", true);
ShaderVariableInfo hero_bbox_reprojectionX("hero_bbox_reprojectionX", true);
ShaderVariableInfo hero_bbox_reprojectionY("hero_bbox_reprojectionY", true);
ShaderVariableInfo hero_bbox_reprojectionZ("hero_bbox_reprojectionZ", true);
} // namespace var

void set_dissolve_frame_counter(const uint16_t sub_samples,
  const uint16_t sub_sample_index,
  const uint16_t super_sample_index,
  const int temporal_shadow_frames_count)
{
  uint32_t tempCount, frameNo;
  if (sub_samples > 1)
  {
    tempCount = sub_samples;
    // offset a bit with super index as well
    frameNo = (sub_sample_index + super_sample_index * 13) % sub_samples;
  }
  else
  {
    tempCount = temporal_shadow_frames_count;
    frameNo = ::dagor_frame_no();
  }
  ShaderGlobal::set_int(var::dissolve_frame_counter, frameNo % tempCount);
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_camera_setup_nodes_es(const OnCameraNodeConstruction &evt)
{
  // TODO: A lot of ifs inside the following nodes should become graph
  // compilation time ifs instead of graph execution time ifs.

  evt.nodes->push_back(dafg::register_node("before_setup_ordering_token_provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.createBlob<OrderingToken>("before_world_render_setup_token");
  }));

  evt.nodes->push_back(dafg::register_node("after_world_render_setup_token_provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.renameBlob<OrderingToken>("before_world_render_setup_token", "after_world_render_setup_token");
  }));

  evt.nodes->push_back(dafg::register_node("setup_world_rendering_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.modifyBlob<OrderingToken>("after_world_render_setup_token");

    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    registry.requestState().setFrameBlock("global_frame");

    if (!shouldRenderGbufferDebug() && !renderer_has_feature(FeatureRenderFlags::POSTFX))
    {
      registry.registerTexture("opaque_resolved", [](const dafg::multiplexing::Index &) -> ManagedTexView {
        G_ASSERTF(!WRDispatcher::isFsrEnabled(), "FSR make no sense without postfx");
        const AntiAliasingMode currentAA = static_cast<AntiAliasingMode>(WRDispatcher::getCurrentAntiAliasingMode());
        if (currentAA != AntiAliasingMode::OFF)
          logerr("Only no-AA will work without postfx: %d", int(currentAA));

        return WRDispatcher::getFinalTargetFrame();
      });
    }
    else
    {
      registry.create("opaque_resolved")
        .texture({get_frame_render_target_format() | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view")});
    }
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("frame_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    struct HandlesToInit
    {
      BlobHandle<bool> belowCloudHndl;
      BlobHandle<WaterRenderMode> waterRenderModeHndl;
      BlobHandle<float> waterLevelHndl;
      BlobHandle<TexStreamingContext> strmCtxHndl;
      BlobHandle<float> gameTimeHndl;
      BlobHandle<float> frameDtHndl;
      BlobHandle<IPoint2> subSuperPixelsHndl;
      // It is used only to keep all drawcalls related to the same sub/super pixel intermediate, otherwise we will override external
      // resources
      BlobHandle<bool> fakeBlobHndl;
      BlobHandle<bool> isUnderwaterHndl;
      BlobHandle<AntiAliasingMode> antiAliasingModeHndl;
      BlobHandle<SunParams> sunParamsHndl;
    };

    return [handles = eastl::shared_ptr<HandlesToInit>(new HandlesToInit{registry.createBlob<bool>("below_clouds").handle(),
              registry.createBlob<WaterRenderMode>("water_render_mode").handle(), registry.createBlob<float>("water_level").handle(),
              registry.createBlob<TexStreamingContext>("tex_ctx").handle(), registry.createBlob<float>("game_time").handle(),
              registry.createBlob<float>("frame_delta_time").handle(), registry.createBlob<IPoint2>("super_sub_pixels").handle(),
              registry.createBlob<bool>("fake_blob").handle(), registry.createBlob<bool>("is_underwater").handle(),
              registry.createBlob<AntiAliasingMode>("anti_aliasing_mode").handle(),
              registry.createBlob<SunParams>("current_sun").handle()})] {
      VolumeLight *volumeLight = WRDispatcher::getVolumeLight();
      if (volumeLight)
        volumeLight->switchOff(); // we should switch off volume light, until it is prepared

      const CameraParams &currentFrameCamera = WRDispatcher::getCurrentCameraParams();
      d3d::settm(TM_WORLD, TMatrix::IDENT);
      d3d::settm(TM_VIEW, currentFrameCamera.viewTm);
      d3d::settm(TM_PROJ, &currentFrameCamera.noJitterProjTm);

      ShaderGlobal::set_int(var::depth_bounds_support, ::depth_bounds_enabled() ? 1 : 0);
      const bool hasUavInVSCapability = false; // TODO get from driver
      if (ShaderGlobal::is_glob_interval_presented(ShaderGlobal::interval<"has_uav_in_vs_capability"_h>))
        ShaderGlobal::set_variant(
          {ShaderGlobal::interval<"has_uav_in_vs_capability"_h>, ShaderGlobal::Subinterval(hasUavInVSCapability ? "yes"_h : "no"_h)});

      const Point3 camPos = currentFrameCamera.viewItm.getcol(3);

      const Point3 curDirToSun = WRDispatcher::getSunDirection();

      Point3 panoramaDirToSun = curDirToSun;
      auto skies = get_daskies();
      if (skies != nullptr)
        panoramaDirToSun = skies->calcPanoramaSunDir(camPos);

      int w, h;
      WRDispatcher::getDisplayResolution(w, h);
      TexStreamingContext currentTexCtx = TexStreamingContext(currentFrameCamera.noJitterPersp, w);

      handles->subSuperPixelsHndl.ref() = WRDispatcher::getSubSuperPixels();
      handles->strmCtxHndl.ref() = currentTexCtx;
      handles->gameTimeHndl.ref() = WRDispatcher::getGameTime();
      handles->frameDtHndl.ref() = WRDispatcher::getRealDeltaTime();
      handles->isUnderwaterHndl.ref() = is_underwater();
      handles->antiAliasingModeHndl.ref() = static_cast<AntiAliasingMode>(WRDispatcher::getCurrentAntiAliasingMode());
      handles->sunParamsHndl.ref() = {curDirToSun, WRDispatcher::getSunColor(), panoramaDirToSun};

      {
        const float water_level = WRDispatcher::GetWaterLevel();
        const bool belowClouds = get_daskies() ? max(camPos.y, water_level) < get_daskies()->getCloudsStartAlt() - 10.0f : true;
        handles->waterLevelHndl.ref() = water_level;
        handles->belowCloudHndl.ref() = belowClouds;

        handles->waterRenderModeHndl.ref() = WRDispatcher::determineWaterRenderMode(handles->isUnderwaterHndl.ref(), belowClouds);
      }
    };
  }));

  evt.nodes->push_back(dafg::register_node("unified_setup_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.orderMeAfter("setup_world_rendering_node");

    registry.executionHas(dafg::SideEffects::External);
    // TODO: Temporary solution, needed to prevent this node being pruned with a logerr. The only consumer of
    // `current_camera_unified` is currently daGdp, which might not be active in all scenes.

    auto cameraHndl = registry.createBlob<CameraParamsUnified>("current_camera_unified").withHistory().handle();

    auto mainPovSkiesDataHndl = registry.create("main_pov_skies_data").blob<SkiesData *>().handle();

    return [cameraHndl, mainPovSkiesDataHndl] {
      // TODO: this is a cheap approximation. In reality, we need a "union" of all multiplexing frustums.
      //
      // It should be fine for SubSampling and SuperSampling multiplexing, as the offsets are very small,
      // and for culling purposes, we can probably ignore them without any apparent visual bugs.
      //
      // But for Viewport multiplexing (which is currently not used in DNG), this needs to be done properly.

      const CameraParams &currentFrameCamera = WRDispatcher::getCurrentCameraParams();

      CameraParamsUnified &camera = cameraHndl.ref();
      camera.unionFrustum = currentFrameCamera.noJitterFrustum;
      camera.cameraWorldPos = currentFrameCamera.cameraWorldPos;
      camera.rangeScale = WRDispatcher::getDaGdpRangeScale();

      mainPovSkiesDataHndl.ref() = WRDispatcher::getMainPovSkiesData();
    };
  }));

  evt.nodes->push_back(dafg::register_node("multisampling_setup_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("setup_world_rendering_node");
    registry.requestState().setFrameBlock("global_frame");

    struct HandlesToInit
    {
      BlobHandle<CameraParams> cameraHndl;
      BlobHandle<ViewVecs> viewVecsHndl;
      BlobHandle<Point4> worldViewPosHndl;
      BlobHandle<SubFrameSample> subFrameSampleHndl;
      BlobHandle<const IPoint2> subSuperPixelsHndl;
      BlobHandle<TMatrix4> motionVecReprojectTmHndl;
    };

    const auto displayResolution = registry.getResolution<2>("display");
    return
      [displayResolution,
        handles = eastl::make_unique<HandlesToInit>(HandlesToInit{
          registry.createBlob<CameraParams>("current_camera").withHistory().handle(),
          registry.createBlob<ViewVecs>("view_vectors").withHistory().handle(), registry.createBlob<Point4>("world_view_pos").handle(),
          registry.createBlob<SubFrameSample>("sub_frame_sample").handle(), registry.readBlob<IPoint2>("super_sub_pixels").handle(),
          registry.createBlob<TMatrix4>("motion_vec_reproject_tm").withHistory().handle()})](
        dafg::multiplexing::Index multiplexing_index) {
        auto [cameraHndl, viewVecsHndl, worldViewPosHndl, subFrameSampleHndl, subSuperPixelsHndl, motionVecReprojectTmHndl] = *handles;

        auto &subFrameSample = subFrameSampleHndl.ref();
        auto [superPixels, subPixels] = subSuperPixelsHndl.ref();

        CameraParams &currentFrameCamera = WRDispatcher::getCurrentCameraParams();
        const CameraParams &prevFrameCamera = WRDispatcher::getPreviousCameraParams();

        auto superSample = multiplexing_index.superSample;
        auto subSample = multiplexing_index.subSample;
        currentFrameCamera.subSampleIndex = subSample;
        currentFrameCamera.subSamples = subPixels * subPixels;
        currentFrameCamera.superSampleIndex = superSample;
        currentFrameCamera.superSamples = superPixels * superPixels;

        set_dissolve_frame_counter(currentFrameCamera.subSamples, multiplexing_index.subSample, multiplexing_index.superSample,
          WRDispatcher::getTemporalShadowFramesCount());

        // NOTE: Last and first are intentionally inverted here, that's
        // just how the hack inside FG works for preserving sequential
        // execution of multiplexing stages.
        // In the future, all of this should be purged and replaced with
        // undermultiplexed nodes.
        if (superPixels == 1 && subPixels == 1)
          subFrameSample = SubFrameSample::Single;
        else if (superSample == 0 && subSample == 0)
          subFrameSample = SubFrameSample::First;
        else if (superSample == superPixels * superPixels - 1 && subSample == subPixels * subPixels - 1)
          subFrameSample = SubFrameSample::Last;
        else
          subFrameSample = SubFrameSample::Intermediate;

        Driver3dPerspective jitterPersp = currentFrameCamera.noJitterPersp;
        auto displayRes = displayResolution.get();

        {
          jitterPersp.ox = jitterPersp.oy = 0;
          if (subPixels > 1)
          {
            const uint32_t subx = subSample / subPixels;
            const uint32_t suby = subSample % subPixels;

            const float ox = (float(subx - 0.5f * subPixels) / subPixels) * (2.0f / (float(displayRes.x * superPixels)));
            const float oy = (float(suby - 0.5f * subPixels) / subPixels) * (2.0f / (float(displayRes.y * superPixels)));

            if (subPixels == 2)
            {
              // rotate by atg( 0.5 ), optimal rotation angle
              constexpr float sinA = 0.4472135954999579;
              constexpr float cosA = 0.8944271909999159;
              jitterPersp.ox = ox * cosA - oy * sinA;
              jitterPersp.oy = ox * sinA + oy * cosA;
            }
            else
            {
              jitterPersp.ox = ox;
              jitterPersp.oy = oy;
            }
          }
          if (superPixels > 1)
          {
            const uint32_t superx = superSample / superPixels;
            const uint32_t supery = superSample % superPixels;

            jitterPersp.ox -= float(superx - 0.5f * superPixels) / superPixels * (2.0f / displayRes.x);
            jitterPersp.oy += float(supery - 0.5f * superPixels) / superPixels * (2.0f / displayRes.y);
          }
        }

        // TODO: we can completely stop camera for being accessible
        // outside of FG and guarantee correct multiplexing behaviour
        // (cameras should differ for different viewports and sub/super samples)

        AntiAliasing *antiAliasing = WRDispatcher::getAntialiasing();
        Point2 jitterOffset(0, 0);
        if (antiAliasing)
          jitterOffset = antiAliasing->update(jitterPersp);

        currentFrameCamera.jitterProjTm = currentFrameCamera.noJitterProjTm;
        currentFrameCamera.jitterProjTm.m[2][0] += jitterPersp.ox;
        currentFrameCamera.jitterProjTm.m[2][1] += jitterPersp.oy;
        auto ndcToUv = [](const Point2 &ndc) { return Point2(ndc.x * 0.5f, -ndc.y * 0.5f); };
        currentFrameCamera.jitterOffsetUv = ndcToUv(jitterOffset);
        // todo: replce that with 'double' version!
        d3d::settm(TM_PROJ, &currentFrameCamera.jitterProjTm);
        d3d::settm(TM_VIEW, currentFrameCamera.viewTm);
        currentFrameCamera.jitterPersp = jitterPersp;
        currentFrameCamera.jitterGlobtm = TMatrix4(currentFrameCamera.viewTm) * currentFrameCamera.jitterProjTm;
        currentFrameCamera.jitterFrustum = currentFrameCamera.jitterGlobtm;
        ShaderGlobal::set_float4(var::uv_temporal_jitter, jitterPersp.ox * 0.5, jitterPersp.oy * -0.5,
          prevFrameCamera.jitterPersp.ox * 0.5, prevFrameCamera.jitterPersp.oy * -0.5);

        TMatrix4D viewRotTm = currentFrameCamera.viewTm;
        viewRotTm.setrow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        currentFrameCamera.viewRotJitterProjTm = TMatrix4(viewRotTm * currentFrameCamera.jitterProjTm);

        const DPoint3 move = currentFrameCamera.cameraWorldPos - prevFrameCamera.cameraWorldPos;
        const ReprojectionTransforms reprojectionTms = calc_reprojection_transforms(prevFrameCamera, currentFrameCamera);
        currentFrameCamera.jitteredCamPosToUnjitteredHistoryClip = reprojectionTms.jitteredCamPosToUnjitteredHistoryClip;

        WRDispatcher::updateTransformations(move, reprojectionTms.jitteredCamPosToUnjitteredHistoryClip,
          reprojectionTms.prevOrigoRelativeViewProjTm);

        auto &camera = cameraHndl.ref();
        camera = currentFrameCamera;
        camera.jobsMgr = WRDispatcher::getMainCameraVisibilityMgr();
        worldViewPosHndl.ref() = Point4::xyz1(camera.viewItm.col[3]);

        camera.viewVecs = calc_view_vecs(camera.viewTm, camera.jitterProjTm);
        viewVecsHndl.ref() = camera.viewVecs;

        motionVecReprojectTmHndl.ref() = reprojectionTms.motionVecReprojectionTm;
      };
  }));

  evt.nodes->push_back(dafg::register_node("hero_matrix_setup_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    auto heroGunOrVehicleTmHndl = registry.createBlob<TMatrix>("hero_gun_or_vehicle_tm").withHistory().handle();
    auto prevHeroGunOrVehicleTmHndl = registry.readBlobHistory<TMatrix>("hero_gun_or_vehicle_tm").handle();
    auto heroMatrixParamsHndl =
      registry.createBlob<eastl::optional<motion_vector_access::HeroMatrixParams>>("hero_matrix_params").handle();

    return [cameraHndl, prevCameraHndl, heroGunOrVehicleTmHndl, prevHeroGunOrVehicleTmHndl, heroMatrixParamsHndl] {
      const DPoint3 &worldPos = cameraHndl.ref().cameraWorldPos;
      const DPoint3 &oldWorldPos = prevCameraHndl.ref().cameraWorldPos;
      mat44f oldHeroGunOrVehicleTm;
      v_mat44_make_from_43cu(oldHeroGunOrVehicleTm, prevHeroGunOrVehicleTmHndl.ref().array);

      HeroWtmAndBox &heroData = WRDispatcher::getHeroData();

      if (heroData.resReady)
      {
        heroData.resWtm.col[3] += Point3(DPoint3(heroData.resWofs) - worldPos);

        const mat44f oldHeroGunOrVehicleNoReprojectedWposTm = oldHeroGunOrVehicleTm;
        oldHeroGunOrVehicleTm.col3 = v_add(oldHeroGunOrVehicleTm.col3,
          v_make_vec4f(oldWorldPos.x - worldPos.x, oldWorldPos.y - worldPos.y, oldWorldPos.z - worldPos.z, 0));

        vec3f lbc = v_bbox3_center(v_ldu_bbox3(heroData.resLbox));
        vec3f lbsize = v_sub(v_ldu_p3(&heroData.resLbox.lim[1].x), lbc);
        ShaderGlobal::set_float(var::gi_hero_cockpit_distance, v_extract_x(v_length3_x(lbsize)));
        lbsize = v_sel(lbsize, v_add(lbsize, lbsize), (vec4f)V_CI_MASK1000); // increase size in one direction, to catch hands

        mat44f resWtm, invHeroGun;
        v_mat44_make_from_43cu(resWtm, heroData.resWtm.m[0]);
        v_mat44_inverse43(invHeroGun, resWtm);

        mat44f boxMatrix, heroMatrix;
        boxMatrix.col0 = v_and(v_div(V_C_UNIT_1000, lbsize), (vec4f)V_CI_MASK1110);
        boxMatrix.col1 = v_and(v_div(V_C_UNIT_0100, lbsize), (vec4f)V_CI_MASK1110);
        boxMatrix.col2 = v_and(v_div(V_C_UNIT_0010, lbsize), (vec4f)V_CI_MASK1110);
        boxMatrix.col3 = v_sel(V_C_ONE, v_sub(v_zero(), v_div(lbc, lbsize)), (vec4f)V_CI_MASK1110);
        v_mat44_mul43(heroMatrix, boxMatrix, invHeroGun);

        mat44f worldToPrev;
        v_mat44_mul43(worldToPrev, oldHeroGunOrVehicleTm, invHeroGun);

        mat43f wtm, wtmPrev;
        v_mat44_transpose_to_mat43(wtmPrev, worldToPrev);
        v_mat44_transpose_to_mat43(wtm, heroMatrix);

        ShaderGlobal::set_float4(var::prev_hero_matrixX, Color4((float *)&wtmPrev.row0));
        ShaderGlobal::set_float4(var::prev_hero_matrixY, Color4((float *)&wtmPrev.row1));
        ShaderGlobal::set_float4(var::prev_hero_matrixZ, Color4((float *)&wtmPrev.row2));

        ShaderGlobal::set_float4(var::hero_matrixX, Color4((float *)&wtm.row0));
        ShaderGlobal::set_float4(var::hero_matrixY, Color4((float *)&wtm.row1));
        ShaderGlobal::set_float4(var::hero_matrixZ, Color4((float *)&wtm.row2));
        ShaderGlobal::set_int(var::hero_is_cockpit, heroData.resFlags == heroData.WEAPON);

        auto toTMatrix = [](const mat43f &m) {
          Point4 rows[3];
          v_stu(&rows[0].x, m.row0);
          v_stu(&rows[1].x, m.row1);
          v_stu(&rows[2].x, m.row2);

          TMatrix tm;
          tm.setcol(0, rows[0].x, rows[1].x, rows[2].x);
          tm.setcol(1, rows[0].y, rows[1].y, rows[2].y);
          tm.setcol(2, rows[0].z, rows[1].z, rows[2].z);
          tm.setcol(3, rows[0].w, rows[1].w, rows[2].w);
          return tm;
        };

        heroMatrixParamsHndl.ref() =
          motion_vector_access::HeroMatrixParams{toTMatrix(wtm), toTMatrix(wtmPrev), heroData.resFlags == heroData.WEAPON};

        mat44f worldToPrevNoReprojectedWPos;
        mat43f wtmPrevNoReprojectedWpos;
        v_mat44_mul43(worldToPrevNoReprojectedWPos, oldHeroGunOrVehicleNoReprojectedWposTm, invHeroGun);
        v_mat44_transpose_to_mat43(wtmPrevNoReprojectedWpos, worldToPrevNoReprojectedWPos);

        ShaderGlobal::set_float4(var::hero_bbox_reprojectionX, Color4((float *)&wtmPrevNoReprojectedWpos.row0));
        ShaderGlobal::set_float4(var::hero_bbox_reprojectionY, Color4((float *)&wtmPrevNoReprojectedWpos.row1));
        ShaderGlobal::set_float4(var::hero_bbox_reprojectionZ, Color4((float *)&wtmPrevNoReprojectedWpos.row2));

        heroGunOrVehicleTmHndl.ref() = heroData.resWtm;
      }
      else
      {
        ShaderGlobal::set_float(var::gi_hero_cockpit_distance, 0);
        ShaderGlobal::set_float4(var::hero_matrixX, 0, 0, 0, 2);
        ShaderGlobal::set_float4(var::hero_matrixY, 0, 0, 0, 2);
        ShaderGlobal::set_float4(var::hero_matrixZ, 0, 0, 0, 2);
        heroGunOrVehicleTmHndl.ref() = prevHeroGunOrVehicleTmHndl.ref();
        heroMatrixParamsHndl.ref() = eastl::nullopt;
      }
    };
  }));
}
