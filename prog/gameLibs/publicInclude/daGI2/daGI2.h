//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <daGI2/settingSupport.h>

class TMatrix;
class TMatrix4;
class BaseTexture;
class Point3;
class BBox3;
struct WorldSDF;

typedef eastl::fixed_function<64, uintptr_t(int clip_no, const BBox3 &box, float voxelSize)> request_sdf_radiance_data_cb;

typedef eastl::fixed_function<64, uintptr_t(const BBox3 &box, float voxelSize)> request_albedo_data_cb;

typedef eastl::fixed_function<64, void(uintptr_t)> cancel_sdf_radiance_data_cb;

typedef eastl::fixed_function<64, void(uintptr_t)> cancel_albedo_data_cb;

enum class UpdateGiQualityStatus
{
  NOTHING,
  RENDERED,
  CANCEL,
  NOT_READY
};

typedef eastl::fixed_function<64, UpdateGiQualityStatus(int clip_no, const BBox3 &box, float voxelSize, uintptr_t &handle)>
  rasterize_sdf_radiance_cb;
typedef eastl::fixed_function<64, UpdateGiQualityStatus(const BBox3 &box, float voxelSize, uintptr_t &handle)> rasterize_albedo_cb;

// if NOT_READY update will be postponed till next frame using same handle, request won't be called again unless cancel is rendered.
// Only case when we reuse handle. if CANCEL update will be restarted on next frame, handle is already invalid and won't be used if
// NOTHING - it was empty handle (no data to render), handle won't be used if RENDERED - data was rendered, handle won't be used again

class DaGI
{
public:
  struct VolumetricGISettings
  {
    uint16_t slices = 16, tileSize = 64;
    uint8_t spatialFilters = 0, radianceRes = 3;
    float historyBlurTexelOfs = 0.125;
    float zDist = 0, zStart = 0.5, zLogMul = 6; // the bigger zLogMul the more linear distribution is
  };

  struct SDFSettings
  {
    uint16_t texWidth = 128;
    uint8_t clips = 5;      // up to 8
    float yResScale = 0.5f; // anisotropy and height direction
    float voxel0Size = 0.15;
    float temporalSpeed = 0.25f;
  };

  struct VoxelScene
  {
    // speed of convergence (amount of pixels for random sampling from gbuffer).
    float temporalSpeed = 0.0625f;
    float sdfResScale = 0.5f; // resolution scale. 0 - means off
    uint8_t firstSDFClip = 1; // all clips below that would be skipped
    uint8_t clips = 0;        // autodetect, sdf.clips - firstSDFClip basically
    bool anisotropy = true;   // 6x more memory, but more accurate in terms of light leaks
    bool onlyLuma = false;    // 2x times less memory, but stores only luma in radiance lit scene
  };

  struct AlbedoScene
  {
    // 0 - means NO albedo scene. This is perfectly legal!
    uint8_t clips = 2;
    float temporalSpeed = 0.0625; // 0...1
    float yResScale = 0.0f;       // <=0 will make it same as SDF yResScale

    // albedo voxel size. no reason it to be smaller than voxelSceneSize
    // 0 - means it is SAME size as HALF voxelScene voxel size (which is sdfResScale*sdfVoxelSize[firstSDFClip])
    float voxel0Size = 0.0f;

    float allocation = 0.0f; // <=0 will make it autodetect
  };

  struct MediaScene
  {
    // 0 - means NO scene. This is perfectly legal!
    uint16_t w = 32, d = 0; // d == 0, means same anisotropy as SDF
    uint8_t clips = 3;
    // albedo voxel size. no reason it to be smaller than voxelSceneSize
    // 0 - means it is SAME size as HALF voxelScene voxel size (which is sdfResScale*sdfVoxelSize[firstSDFClip])
    float voxel0Size = 0.0f;
    float temporalSpeed = 0.125; // 0 means off, 1 - maximum temporality
  };

  struct ScreenProbesSettings
  {
    uint8_t tileSize = 16;
    uint8_t radianceOctRes = 8; // 4..16
    bool angleFiltering = false;
    float temporality = 1.f; // 1 - each probe updates each frame.
    uint8_t quality = 0;
    float overAllocation = 0.75; // should be used instead of console command
  };

  struct RadianceGridSettings
  {
    // speed of convergence (amount of pixels for random sampling from gbuffer).
    uint8_t w = 32, d = 0; // 0 - autodetect
    uint8_t clips = 3;
    uint8_t framesToUpdateOneClip = 16; // temporality
    float irradianceProbeDetail = 1.f;  // 1+, 1 - means same as radiance grid. Much faster option (if with additionalIrradianceClips =
                                        // 0).
    uint8_t additionalIrradianceClips = 0; // amount of additional clips in radiance grid.
  };

  // begin not used
  struct SkyVisibilitySettings
  {
    uint8_t clipW = 0, clips = 5;
    float yResScale = 0.f;                  // == 0, will autodetect based on SDF
    uint32_t framesToUpdateTemporally = 32; // no
    float traceDist = 64.f;                 // max trace dist in meters. 0 - will autodetect based on SDF
    float upscaleInProbes = 6.f; // when tracing finer cascades, we will emit shorter rays based and use interpolated coarser cascade
                                 // info. Distance in probes of next cascade.
    float airScale = 0.75f;      // 'air' probes (which not intersect any static surface) will use shorter rays
    uint32_t simulateLightBounceClips = 3; // all cascades <= this would simulate bounce. Simulated bounce introduces more light
                                           // leaking, but more accurate in lighting
  };

  struct RadianceCacheSettings
  {
    uint32_t probesMin = 16384;
    float probesMax = 1.0f;
    float probe0Size = 0.0f; // <=0 will autodetect
    float yResScale = 0.0f;  // <=0 will autodetect
    uint16_t xzRes = 64;
    uint8_t clips = 3;
    uint16_t updateInFrames = 16;     // temporality how many frames to update all active probes
    uint16_t updateOldProbeFreq = 4;  // will skip each N-1 frames for non-active probes. Non-active probes will be updated in each
                                      // updateInFrames*updateOldProbeFreq frames
    uint8_t probeFramesKeep = 4;      // new probe will be kept at least that amount of frames
    bool screenCast = true;           // uses prev resolved frame
    bool allowDebugAllocation = true; //
    bool updateAllClips = true;       // to relax cost of movement / teleportation with a certain latency
  };
  // end not used

  struct Settings
  {
    SDFSettings sdf;
    AlbedoScene albedoScene;
    VoxelScene voxelScene;
    MediaScene mediaScene;
    ScreenProbesSettings screenProbes;
    VolumetricGISettings volumetricGI;
    RadianceGridSettings radianceGrid;
    SkyVisibilitySettings skyVisibility;

    // not used
    RadianceCacheSettings radianceCache;
  };

  virtual ~DaGI() {}
  virtual void fix(Settings &settings) const = 0;
  virtual void debugRenderScreenDepth() = 0;
  virtual void debugRenderScreen() = 0;
  virtual void debugRenderTrans() = 0;
  virtual bool sdfClipHasVoxelsLitSceneClip(uint32_t sdf_clip) const = 0;
  virtual void requestUpdatePosition(const request_sdf_radiance_data_cb &sdf_cb, const cancel_sdf_radiance_data_cb &cancel_sdf_cb,
    const request_albedo_data_cb &albedo_cb, const cancel_albedo_data_cb &cancel_albedo_cb) = 0;
  virtual void updatePosition(const rasterize_sdf_radiance_cb &sdf_cb, const rasterize_albedo_cb &albedo_cb) = 0;
  virtual void beforeRender(uint32_t screen_w, uint32_t screen_h, uint32_t max_screen_w, uint32_t max_screen_h, const TMatrix &viewItm,
    const TMatrix4 &projTm, float zn, float zf) = 0;
  virtual void setSettings(const Settings &s) = 0;
  virtual Settings getCurrentSettings() const = 0;
  virtual Settings getNextSettings() const = 0;
  virtual const WorldSDF &getWorldSDF() const = 0;
  virtual WorldSDF &getWorldSDF() = 0;
  virtual void afterReset() = 0;
  enum FrameData
  {
    FrameHasDepth = 1 << 0,
    FrameHasDynamic = 1 << 1, // we can update sdf and others safely, is_dynamic present
    FrameHasAlbedo = 1 << 2,
    FrameHasLitScene = 1 << 3,
    FrameHasAll = 0xFF,
  };
  // makes sdf from gbuffer
  // updates GI data from gbuffer. if FrameHasAlbedo (deferred!), albedoScene will be updated
  // updates GI data from (resolved) gbuffer.
  //  if FrameHasLitScene, voxelLitScene will be updated. You can update using lit data (rendered frame),
  //    it is cheaper, but will include specular and may be scattering (depends)
  //  You can also update it using gbuffer (rendered frame)
  virtual void afterFrameRendered(FrameData frame_featues) = 0;

  virtual void beforeFrameLit(float quality = 1.0f) = 0; // if quality is 0, we calculate GI with a same algorithm, but as fast as
                                                         // possible
};

DaGI *create_dagi();
