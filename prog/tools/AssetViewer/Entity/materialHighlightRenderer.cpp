// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "materialHighlightRenderer.h"
#include <de3_baseInterfaces.h>
#include <de3_dynRenderService.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_randomSeed.h>
#include <EditorCore/ec_interface.h>
#include <math/dag_color.h>
#include <math/dag_TMatrix4.h>
#include <perfMon/dag_statDrv.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/riShaderConstBuffers.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/dag_shaders.h>
#include <util/dag_bitwise_cast.h>
#include <vecmath/dag_vecMath.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

static ShaderVariableInfo rendinstUseCellSbufferVarInfo("rendinst_use_cell_sbuffer", true);
static ShaderVariableInfo depthFadeTintColorVarInfo("depth_fade_tint_color", true);
static ShaderVariableInfo depthFadeTintMaskDepthVarInfo("depth_fade_tint_mask_depth", true);
static ShaderVariableInfo depthFadeTintSceneDepthVarInfo("depth_fade_tint_scene_depth", true);
static ShaderVariableInfo depthFadeTintIsOrthoVarInfo("depth_fade_tint_is_ortho", true);
static ShaderVariableInfo depthFadeTintFadeDepthDistanceVarInfo("depth_fade_tint_fade_depth_distance", true);

static constexpr float PERSPECTIVE_FADE_DISTANCE = 0.001f; // fraction of the view distance
static constexpr float ORTHO_FADE_DEPTH = 1e-7f;           // band in raw depth units

static void copy_elems_of_material(ShaderMesh &dst, const ShaderMesh &src, const ShaderMaterial &material)
{
  dag::Span<ShaderMesh::RElem> dstElems = dst.getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_COUNT - 1);
  dag::Span<ShaderMesh::RElem> srcElems = src.getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_COUNT - 1);
  G_ASSERT_RETURN(dstElems.size() == srcElems.size(), );
  for (int i = 0; i < dstElems.size(); ++i)
  {
    dstElems[i].mat = srcElems[i].mat;
    dstElems[i].e = srcElems[i].mat.get() == &material ? srcElems[i].e : nullptr;
  }
}

static void hide_all_elems(DynamicRenderableSceneLodsResource &lods_res)
{
  for (DynamicRenderableSceneLodsResource::Lod &lod : lods_res.lods)
  {
    for (DynamicRenderableSceneResource::RigidObject &rigid : lod.scene->getRigids())
      for (ShaderMesh::RElem &elem : rigid.mesh->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_COUNT - 1))
        elem.e = nullptr;
    for (ShaderSkinnedMeshResource *skin : lod.scene->getSkins())
      for (ShaderMesh::RElem &elem : skin->getMesh()->getShaderMesh().getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_COUNT - 1))
        elem.e = nullptr;
  }
}

MaterialHighlightRenderer::~MaterialHighlightRenderer() { end(); }

void MaterialHighlightRenderer::begin(IObjEntity *asset_entity)
{
  end();
  entity = asset_entity;
}

void MaterialHighlightRenderer::render(Mode mode, const Color4 &color, const ShaderMaterial &material, int lod,
  rendinst::riex_handle_t riex_override)
{
  if (!entity)
    return;

  if (IEntityGetDynSceneLodsRes *dynRes = entity->queryInterface<IEntityGetDynSceneLodsRes>())
  {
    if (const DynamicRenderableSceneInstance *instance = dynRes->getSceneInstance())
      renderDynModel(mode, color, *instance, material, lod);
    return;
  }

  IEntityGetRISceneLodsRes *riRes = entity->queryInterface<IEntityGetRISceneLodsRes>();
  const RenderableInstanceLodsResource *lodsRes = riRes ? riRes->getSceneLodsRes() : nullptr;
  if (!lodsRes)
    return;

  TMatrix tm;
  uint32_t instSeed = 0;
  if (riex_override != rendinst::RIEX_HANDLE_NULL)
  {
    mat44f riexTm;
    rendinst::getRIGenExtra44(riex_override, riexTm);
    v_mat_43cu_from_mat44(tm.array, riexTm);
    instSeed = rendinst::get_riextra_instance_seed(riex_override);
  }
  else
  {
    IRendInstEntity *rendInst = entity->queryInterface<IRendInstEntity>();
    if (!rendInst || !rendInst->getRendInstQuantizedTm(tm))
      entity->getTm(tm);
    if (IRandomSeedHolder *seedHolder = entity->queryInterface<IRandomSeedHolder>())
      instSeed = seedHolder->getSeed();
  }
  renderRendInst(mode, color, *lodsRes, lod, tm, instSeed, material);
}

void MaterialHighlightRenderer::end()
{
  resetFilteredDynModel();
  entity = nullptr;
}

void MaterialHighlightRenderer::resetFilteredDynModel()
{
  if (!filteredLodsRes)
    return;
  // Hide the elems so the instance does not release texture refs it never acquired
  hide_all_elems(*filteredLodsRes);
  filteredInstance.reset();
  filteredLodsRes = nullptr;
  filteredSource = nullptr;
  filteredMaterial = nullptr;
  filteredLod = -1;
}

bool MaterialHighlightRenderer::init()
{
  if (initState != InitState::NotDone)
    return initState == InitState::Done;

  maskRender.init("depth_fade_tint");
  if (!maskRender.getMat())
  {
    DAEDITOR3.conError("Shader \"depth_fade_tint\" cannot be found. Material highlight won't work!");
    initState = InitState::Failed;
    return false;
  }

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_TEST_ENABLE | shaders::OverrideState::Z_WRITE_ENABLE | shaders::OverrideState::Z_FUNC |
            shaders::OverrideState::CULL_NONE);
  state.zFunc = CMPF_GREATEREQUAL;
  maskOverride = shaders::overrides::create(state);

  initState = InitState::Done;
  return true;
}

bool MaterialHighlightRenderer::beginMask()
{
  if (!init())
    return false;

  d3d::get_render_target(prevRt);
  BaseTexture *target = prevRt.isColorUsed(0) ? prevRt.getColor(0).tex : nullptr;
  if (!target)
    return false;

  TextureInfo targetInfo;
  target->getinfo(targetInfo);
  TextureInfo maskInfo;
  if (maskDepth)
    maskDepth->getinfo(maskInfo);
  if (!maskDepth || maskInfo.w != targetInfo.w || maskInfo.h != targetInfo.h)
    maskDepth =
      dag::create_tex(nullptr, targetInfo.w, targetInfo.h, TEXFMT_DEPTH32 | TEXCF_RTARGET, 1, "material_highlight_mask_depth");

  d3d::set_render_target({maskDepth.getTex2D(), 0, 0}, DepthAccess::RW, {});
  d3d::clearview(CLEAR_ZBUFFER, 0, 0.0f, 0);

  static const int globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
  prevFrameBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  prevOverride = shaders::overrides::get_current();
  if (prevOverride)
    shaders::overrides::reset();
  shaders::overrides::set(maskOverride.get());
  return true;
}

void MaterialHighlightRenderer::endMask(Mode mode, const Color4 &color)
{
  shaders::overrides::reset();
  if (prevOverride)
    shaders::overrides::set(prevOverride);
  ShaderGlobal::setBlock(prevFrameBlock, ShaderGlobal::LAYER_FRAME);

  const Driver3dRenderTarget::RTState &target = prevRt.getColor(0);
  BaseTexture *sceneDepth = prevRt.isDepthUsed() ? prevRt.getDepth().tex : nullptr;
  // Use maskDepth as occluder to always pass the depth check in depth_fade_tint for simple mode
  BaseTexture *occluderDepth = mode == Mode::DepthAware && sceneDepth ? sceneDepth : maskDepth.getTex2D();
  d3d::set_render_target({}, DepthAccess::RW, {{target.tex, target.level, target.layer}});

  depthFadeTintColorVarInfo.set_float4(color);
  depthFadeTintMaskDepthVarInfo.set_texture(maskDepth.getTex2D());
  depthFadeTintSceneDepthVarInfo.set_texture(occluderDepth);
  TMatrix4 proj;
  d3d::gettm(TM_PROJ, &proj);
  const bool isOrtho = proj._34 == 0;
  depthFadeTintIsOrthoVarInfo.set_int(isOrtho ? 1 : 0);
  depthFadeTintFadeDepthDistanceVarInfo.set_float(isOrtho ? ORTHO_FADE_DEPTH : PERSPECTIVE_FADE_DISTANCE);
  maskRender.render();
  depthFadeTintMaskDepthVarInfo.set_texture(BAD_TEXTUREID);
  depthFadeTintSceneDepthVarInfo.set_texture(BAD_TEXTUREID);

  d3d::set_render_target(prevRt);
}

bool MaterialHighlightRenderer::syncFilteredDynModel(const DynamicRenderableSceneInstance &instance, const ShaderMaterial &material,
  int lod)
{
  const DynamicRenderableSceneLodsResource *source = instance.getLodsResource();
  if (!source || lod < 0 || lod >= source->lods.size())
    return false;

  if (source != filteredSource)
  {
    resetFilteredDynModel();
    filteredLodsRes = source->clone();
    filteredLodsRes->updateBindposes();
    hide_all_elems(*filteredLodsRes);
    filteredInstance = eastl::make_unique<DynamicRenderableSceneInstance>(filteredLodsRes.get());
    filteredSource = source;
  }

  if (lod == filteredLod && &material == filteredMaterial)
    return true;

  const DynamicRenderableSceneResource &srcScene = *source->lods[lod].scene;
  DynamicRenderableSceneResource &dstScene = *filteredLodsRes->lods[lod].scene;
  dag::ConstSpan<DynamicRenderableSceneResource::RigidObject> srcRigids = srcScene.getRigidsConst();
  dag::Span<DynamicRenderableSceneResource::RigidObject> dstRigids = dstScene.getRigids();
  for (int i = 0; i < dstRigids.size(); ++i)
    copy_elems_of_material(*dstRigids[i].mesh->getMesh(), *srcRigids[i].mesh->getMesh(), material);
  for (int i = 0; i < dstScene.getSkins().size(); ++i)
  {
    ShaderSkinnedMeshResource *dstSkin = dstScene.getSkins()[i];
    copy_elems_of_material(dstSkin->getMesh()->getShaderMesh(), srcScene.getSkins()[i]->getMesh()->getShaderMesh(), material);
  }
  filteredLod = lod;
  filteredMaterial = &material;
  return true;
}

void MaterialHighlightRenderer::renderDynModel(Mode mode, const Color4 &color, const DynamicRenderableSceneInstance &instance,
  const ShaderMaterial &material, int lod)
{
  IDynRenderService *rs = EDITORCORE->queryEditorInterface<IDynRenderService>();
  if (!rs)
    return;

  TIME_D3D_PROFILE(material_highlight_dynmodel);
  if (!syncFilteredDynModel(instance, material, lod))
    return;
  filteredInstance->setLod(lod);
  if (filteredInstance->getCurrentLodNo() != lod)
    return;
  filteredInstance->setOrigin(instance.getOrigin());
  filteredInstance->setOriginPrev(instance.getOriginPrev());
  for (uint32_t i = 0, n = instance.getNodeCount(); i < n; ++i)
  {
    filteredInstance->setNodeWtm(i, instance.getNodeWtm(i));
    filteredInstance->setPrevNodeWtm(i, instance.getPrevNodeWtm(i));
    filteredInstance->showNode(i, !instance.getNodeHidden(i));
    filteredInstance->setNodeOpacity(i, instance.getNodeOpacity(i));
  }

  IRandomSeedHolder *seedHolder = entity->queryInterface<IRandomSeedHolder>();
  int instSeed = seedHolder ? seedHolder->getPerInstanceSeed() : 0;

  if (!beginMask())
    return;
  for (IRenderingService::Stage stage : {IRenderingService::STG_RENDER_DYNAMIC_OPAQUE, IRenderingService::STG_RENDER_DYNAMIC_DECALS,
         IRenderingService::STG_RENDER_DYNAMIC_TRANS, IRenderingService::STG_RENDER_DYNAMIC_DISTORTION})
    rs->renderOneDynModelInstance(filteredInstance.get(), stage, &instSeed);
  endMask(mode, color);
}

void MaterialHighlightRenderer::renderRendInst(Mode mode, const Color4 &color, const RenderableInstanceLodsResource &lods_res, int lod,
  const TMatrix &tm, uint32_t inst_seed, const ShaderMaterial &material)
{
  if (lod < 0 || lod >= lods_res.lods.size())
    return;

  if (!lods_res.lods[lod].scene)
    return;

  const InstShaderMeshResource *meshRes = lods_res.lods[lod].scene->getMesh();
  if (!meshRes)
    return;

  const InstShaderMesh *instMesh = meshRes->getMesh();
  if (!instMesh)
    return;

  const ShaderMesh *mesh = instMesh->getMesh();
  if (!mesh)
    return;

  TIME_D3D_PROFILE(material_highlight_rendinst);
  if (!rendinstMatrixBuffer)
    rendinstMatrixBuffer = ShaderGlobal::get_interval_current_value(rendinstUseCellSbufferVarInfo) == 1
                             ? dag::buffers::create_persistent_sr_structured(sizeof(Point4), 4, "material_highlight_matrix_buffer")
                             : dag::buffers::create_persistent_sr_tbuf(4, TEXFMT_A32B32G32R32F, "material_highlight_matrix_buffer");
  if (!rendinstMatrixBuffer)
    return;

  // the shader reads hashVal from .z
  const Point4 matrixData[4] = {
    Point4(tm.getcol(0).x, tm.getcol(1).x, tm.getcol(2).x, tm.getcol(3).x),
    Point4(tm.getcol(0).y, tm.getcol(1).y, tm.getcol(2).y, tm.getcol(3).y),
    Point4(tm.getcol(0).z, tm.getcol(1).z, tm.getcol(2).z, tm.getcol(3).z),
    Point4(0, 0, bitwise_cast<float>(inst_seed), 0),
  };
  rendinstMatrixBuffer->updateData(0, sizeof(matrixData), matrixData, 0);

  if (!beginMask())
    return;

  {
    // tm_vb of the instancing_type shader interval
    static constexpr int COORD_TYPE_TM = 0;
    static const int rendinstRenderPassVarId = ::get_shader_glob_var_id("rendinst_render_pass");
    static const int rendinstSceneBlockId = ShaderGlobal::getBlockId("rendinst_scene");
    SCENE_LAYER_GUARD(rendinstSceneBlockId);

    rendinst::render::startRenderInstancing();
    static const int instancingTypeVarId = ::get_shader_glob_var_id("instancing_type");
    const int prevInstancingType = ShaderGlobal::get_int(instancingTypeVarId);
    ShaderGlobal::set_int(instancingTypeVarId, COORD_TYPE_TM);
    const int prevRenderPass = ShaderGlobal::get_int(rendinstRenderPassVarId);
    ShaderGlobal::set_int(rendinstRenderPassVarId, eastl::to_underlying(rendinst::RenderPass::Normal));
    d3d::setind(unitedvdata::riUnitedVdata.getIB());
    d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rendinstMatrixBuffer.getBuf());

    rendinst::render::RiShaderConstBuffers cb;
    cb.setBBoxZero();
    cb.setOpacity(0, 1);
    cb.setBoundingSphere(0, 0, 1, 1, 0);
    cb.setInstancing(0, 4, RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_CONST_BUFFER | RI_CBUFFER_FLAGS__HASH_VAL, 0);
    cb.flushPerDraw();
    d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

    for (const ShaderMesh::RElem &elem : mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_distortion))
    {
      if (elem.mat.get() == &material && elem.e && !elem.vertexData->isEmpty())
      {
        elem.vertexData->setToDriver();
        elem.render();
      }
    }

    ShaderGlobal::set_int(rendinstRenderPassVarId, prevRenderPass);
    ShaderGlobal::set_int(instancingTypeVarId, prevInstancingType);
    rendinst::render::endRenderInstancing();
  }
  endMask(mode, color);
}
