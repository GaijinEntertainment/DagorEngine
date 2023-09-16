//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <generic/dag_staticTab.h> // TODO: replace unsafe StaticTab
#include <math/dag_TMatrix4.h>
#include <textureUtil/textureUtil.h>
#include <EASTL/array.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <3d/dag_resPtr.h>

#include <math/dag_hlsl_floatx.h>
#include "heightFogNode.hlsli"
#include <render/viewDependentResource.h>
#include <shaders/dag_postFxRenderer.h>


class ComputeShaderElement;
class DataBlock;
class NodeBasedShader;

class VolumeLight
{
public:
  VolumeLight();
  ~VolumeLight();
  void close();
  void init();
  void setResolution(int resW, int resH, int resD, int screenW, int screenH);
  void getResolution(int &resW, int &resH, int &resD)
  {
    resW = froxelResolution[0];
    resH = froxelResolution[1];
    resD = froxelResolution[2];
  }
  void setRange(float range);
  void setCurrentView(int view);
  bool perform(const TMatrix4 &view_tm, const TMatrix4_vec4 &glob_tm, const Point3 &camera_pos); // true if on
  void performIntegration();
  void switchOff();
  void switchOn();
  void closeShaders();
  void beforeReset();
  void afterReset();
  void invalidate();

  bool updateShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors);
  void initShaders(const DataBlock &shader_blk);
  void enableOptionalShader(const String &shader_name, bool enable);
  const UniqueTexHolder &getInitialMedia() const { return initialMedia; }
  void onSettingsChange(bool has_hq_fog, bool has_distant_fog, bool has_volfog_shadow);

  void onCompatibilityModeChange(bool use_compatiblity_mode); // TODO: use ECS to handle compatiblity preset change

  Frustum calcFrustum(const TMatrix4 &view_tm, const Driver3dPerspective &persp) const;

  bool isDistantFogEnabled() const;

protected:
  static constexpr int FRAME_HISTORY = 2;
  static constexpr int DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT = 5;
  static constexpr float DISTANT_FOG_OCCLUSION_WEIGHT_MAX_SAMPLE_OFFSET = 1 << (DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT - 1);

  void initFroxelFog();
  void initDistantFog();
  void resetDistantFog();
  void initVolfogShadow();
  void resetVolfogShadow();

  int getBlendedSliceCnt() const;
  float calcBlendedStartDepth(int blended_slice_cnt) const;
  bool canUseLinearAccumulation() const;
  bool canUsePixelShader() const;
  bool isVolfogShadowEnabled() const;

  void performRaymarching(bool use_node_based_input, const TMatrix4 &view_tm, const Point3 &pos_diff);

  bool hasDistantFog() const;
  bool hasVolfogShadows() const;

  eastl::array<UniqueTex, FRAME_HISTORY> initialInscatter;
  eastl::array<UniqueTex, FRAME_HISTORY> initialExtinction;

  UniqueTexHolder initialMedia;
  UniqueTexHolder resultInscatter;
  UniqueTexHolder initialPhase; // for local lights (if present)
  eastl::array<UniqueTex, FRAME_HISTORY> volfogOcclusion;

  eastl::array<UniqueTex, FRAME_HISTORY> volfogShadow;

  UniqueTexHolder poissonSamples;

  UniqueTexHolder distantFogFrameRaymarchInscatter;
  UniqueTexHolder distantFogFrameRaymarchExtinction;
  UniqueTexHolder distantFogFrameRaymarchDist;
  UniqueTexHolder distantFogFrameRaymarchDebug;

  // for FX and other transparent shaders only,
  // uses half res of reconstruction (quad res by default)
  UniqueTexHolder distantFogFrameReprojectionDist;

  eastl::array<UniqueTex, FRAME_HISTORY> distantFogFrameReconstruct;
  eastl::array<UniqueTex, FRAME_HISTORY> distantFogRaymarchStartWeights;
  eastl::array<UniqueTex, FRAME_HISTORY> distantFogFrameReconstructionWeight;

  UniqueTexHolder distantFogDebug;

  eastl::unique_ptr<ComputeShaderElement> volfogOcclusionCs;
  eastl::unique_ptr<ComputeShaderElement> viewInitialInscatterCs;
  eastl::unique_ptr<ComputeShaderElement> fillMediaCs;
  eastl::unique_ptr<ComputeShaderElement> viewResultInscatterCs;
  eastl::unique_ptr<ComputeShaderElement> volfogShadowCs;
  eastl::unique_ptr<ComputeShaderElement> clearVolfogShadowCs;

  PostFxRenderer viewResultInscatterPs;

  eastl::unique_ptr<ComputeShaderElement> distantFogRaymarchCs;
  eastl::unique_ptr<ComputeShaderElement> distantFogReconstructCs;
  eastl::unique_ptr<ComputeShaderElement> distantFogMipGenerationCs;
  eastl::unique_ptr<ComputeShaderElement> distantFogFxMipGenerationCs;

  eastl::unique_ptr<NodeBasedShader> nodeBasedFillMediaCs;
  eastl::unique_ptr<NodeBasedShader> nodeBasedDistantFogRaymarchCs;
  eastl::unique_ptr<NodeBasedShader> nodeBasedFogShadowCs;

  eastl::shared_ptr<texture_util::ShaderHelper> shaderHelperUtils;

  IPoint3 froxelOrigResolution = IPoint3::ZERO;
  IPoint3 froxelResolution = IPoint3::ZERO;
  IPoint2 raymarchFrameRes = IPoint2::ZERO;
  IPoint2 reconstructionFrameRes = IPoint2::ZERO;
  float currentRange = 128; // arbitrary, setRange should be called to not rely on it
  Point3 prevCameraPos = IPoint3::ZERO;

  ViewDependentResource<TMatrix4_vec4, 2, 1> prevGlobTm = ViewDependentResource<TMatrix4_vec4, 2, 1>(TMatrix4_vec4::ZERO);
  int frameId = 0;
  int raymarchFrameId = 0;
  int froxelChannelSwizzleFrameId = 0;
  float prevRange = 0;

  bool fogIsValid = false; // was ever performed
  bool isReady = false;
  bool useCompatibilityMode = false;
  bool preferLinearAccumulation = false;
  bool hq_volfog = false;
};
