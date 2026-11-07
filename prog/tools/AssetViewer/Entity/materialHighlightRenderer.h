// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_decl.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_postFxRenderer.h>
#include <rendInst/riexHandle.h>
#include <EASTL/unique_ptr.h>

class IObjEntity;
class RenderableInstanceLodsResource;
class ShaderMaterial;
class TMatrix;
struct Color4;

class MaterialHighlightRenderer
{
public:
  enum class Mode
  {
    None,
    Simple,     // visible through other geometry
    DepthAware, // fades out behind the scene geometry
    Count
  };

  ~MaterialHighlightRenderer();

  void begin(IObjEntity *entity);
  // riex_override replaces the entity TM and seed when set (rendinst entities only)
  void render(Mode mode, const Color4 &color, const ShaderMaterial &material, int lod,
    rendinst::riex_handle_t riex_override = rendinst::RIEX_HANDLE_NULL);
  void end();

private:
  void renderDynModel(Mode mode, const Color4 &color, const DynamicRenderableSceneInstance &instance, const ShaderMaterial &material,
    int lod);
  void renderRendInst(Mode mode, const Color4 &color, const RenderableInstanceLodsResource &lods_res, int lod, const TMatrix &tm,
    uint32_t inst_seed, const ShaderMaterial &material);
  bool init();
  bool beginMask();
  void endMask(Mode mode, const Color4 &color);
  void resetFilteredDynModel();
  bool syncFilteredDynModel(const DynamicRenderableSceneInstance &instance, const ShaderMaterial &material, int lod);

  enum class InitState
  {
    NotDone,
    Done,
    Failed,
  };
  InitState initState = InitState::NotDone;
  PostFxRenderer maskRender;
  shaders::UniqueOverrideStateId maskOverride;
  shaders::OverrideStateId prevOverride;
  int prevFrameBlock = -1;
  Driver3dRenderTarget prevRt;
  UniqueTex maskDepth;
  UniqueBuf rendinstMatrixBuffer;

  IObjEntity *entity = nullptr;

  const DynamicRenderableSceneLodsResource *filteredSource = nullptr;
  const ShaderMaterial *filteredMaterial = nullptr;
  int filteredLod = -1;
  Ptr<DynamicRenderableSceneLodsResource> filteredLodsRes;
  eastl::unique_ptr<DynamicRenderableSceneInstance> filteredInstance;
};
