// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <dag/dag_vectorMap.h>
#include <generic/dag_initOnDemand.h>
#include <math/dag_frustum.h>
#include <memory/dag_framemem.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_texStreamingContext.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_shStateBlockBindless.h>
#include <scene/dag_occlusion.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/riShaderConstBuffers.h>
#include <rendInst/packedMultidrawParams.hlsli>

#include <daFracture/render/renderContext.h>
#include <daFracture/render/renderList.h>


namespace rendinst::render
{

enum CoordType
{
  COORD_TYPE_TM = 0,
  COORD_TYPE_POS = 1,
  COORD_TYPE_POS_CB = 2
};

extern ShaderBlockIdHolder rendinstSceneBlockId;
extern ShaderBlockIdHolder rendinstSceneTransBlockId;
extern ShaderBlockIdHolder rendinstDepthSceneBlockId;
extern int rendinstRenderPassVarId;
extern int instancingTexRegNo;
extern void setCoordType(CoordType type);
} // namespace rendinst::render


namespace frx
{

void render_simple_dyn_meshes(const MeshRenderList &list, RenderPass render_pass)
{
  G_ASSERT_RETURN(list.curState == MeshRenderList::PrepareState::PREPARED, );
  if (list.elems.empty())
    return;

  ShaderMesh::Stage stageFrom = ShaderMesh::STG_opaque, stageTo = ShaderMesh::STG_opaque;
  switch (render_pass)
  {
    case RenderPass::DEPTH:
    case RenderPass::OPAQUE:
      stageFrom = ShaderMesh::STG_opaque;
      stageTo = ShaderMesh::STG_atest;
      break;
    case RenderPass::IMM_DECAL:
      stageFrom = ShaderMesh::STG_imm_decal;
      stageTo = ShaderMesh::STG_imm_decal;
      break;
    case RenderPass::DECAL:
      stageFrom = ShaderMesh::STG_decal;
      stageTo = ShaderMesh::STG_decal;
      break;
    case RenderPass::TRANSPARENT:
      stageFrom = ShaderMesh::STG_trans;
      stageTo = ShaderMesh::STG_trans;
      break;
  }

  for (auto &elem : list.getElems(stageFrom, stageTo))
  {
    d3d::settm(TM_WORLD, list.insts[elem.instId].tm);
    if (!elem.shElem->setStates())
      G_ASSERT_CONTINUE(0);
    d3d_err(d3d::setind(elem.ib));
    d3d_err(d3d::setvsrc(0, elem.vb, elem.vStride));
    d3d::drawind(PRIM_TRILIST, elem.ibOfs, elem.fCnt, elem.vbOfs);
  }
  d3d::settm(TM_WORLD, TMatrix::IDENT);
}

void render_ri_meshes(const MeshRenderList &list, RenderPass render_pass)
{
  G_ASSERT_RETURN(list.curState == MeshRenderList::PrepareState::RI, );
  if (list.elems.empty())
    return;

  FRAMEMEM_REGION;
  TIME_D3D_PROFILE(frx_render_ri_meshes)

  ShaderMesh::Stage stageFrom = ShaderMesh::STG_opaque, stageTo = ShaderMesh::STG_opaque;
  rendinst::RenderPass riRenderPass = rendinst::RenderPass::Normal;
  int sceneBlock = rendinst::render::rendinstSceneBlockId;
  switch (render_pass)
  {
    case RenderPass::DEPTH:
      riRenderPass = rendinst::RenderPass::Depth;
      sceneBlock = rendinst::render::rendinstDepthSceneBlockId;
      stageFrom = ShaderMesh::STG_opaque;
      stageTo = ShaderMesh::STG_atest;
      break;
    case RenderPass::OPAQUE:
      stageFrom = ShaderMesh::STG_opaque;
      stageTo = ShaderMesh::STG_atest;
      break;
    case RenderPass::IMM_DECAL:
      stageFrom = ShaderMesh::STG_imm_decal;
      stageTo = ShaderMesh::STG_imm_decal;
      break;
    case RenderPass::DECAL:
      stageFrom = ShaderMesh::STG_decal;
      stageTo = ShaderMesh::STG_decal;
      break;
    case RenderPass::TRANSPARENT:
      sceneBlock = rendinst::render::rendinstSceneTransBlockId;
      stageFrom = ShaderMesh::STG_trans;
      stageTo = ShaderMesh::STG_trans;
      break;
  }

  rendinst::render::startRenderInstancing();
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);
  rendinst::render::RiShaderConstBuffers cb;
  cb.setBBoxZero();
  cb.setInstancing(0, 4,
    RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_CONST_BUFFER | RI_CBUFFER_FLAGS__HASH_VAL | RI_CBUFFER_FLAGS__INITIAL_POS, 0);
  cb.flushPerDraw();

  SCENE_LAYER_GUARD(sceneBlock);
  ShaderGlobal::set_int(rendinst::render::rendinstRenderPassVarId, eastl::to_underlying(riRenderPass));
  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, list.perFrameDataBuf.getBuf());

  using RElem = MeshRenderList::RElem;
  const auto elems = list.getElems(stageFrom, stageTo);

  struct MultidrawElem
  {
    const RElem *re;
    shaders::CombinedDynVariantState dynVarsState;
  };
  dag::Vector<const RElem *, framemem_allocator> immediateElems;
  dag::Vector<MultidrawElem, framemem_allocator> multidrawElems;
  dag::VectorMap<shaders::ConstStateIdx, int, eastl::less<shaders::ConstStateIdx>, framemem_allocator> bindlessTexLevels;
  immediateElems.reserve(elems.size());
  multidrawElems.reserve(elems.size());

  // prepare lists
  {
    ShaderElement *curSElem = nullptr;
    bool curIsMultidraw = false;
    shaders::CombinedDynVariantState curDynVarsState = {};
    for (const RElem &re : elems)
    {
      if (re.shElem != curSElem)
      {
        curSElem = re.shElem;
        curDynVarsState = shaders::get_dynamic_variant_state(re.shElem->native());
        curIsMultidraw = is_packed_material(curDynVarsState.const_state);
        if (curIsMultidraw)
          // TODO: calculate actual tex level in MeshRenderList::prepare
          bindlessTexLevels.emplace(curDynVarsState.const_state, TexStreamingContext::MAX_TEX_LEVEL);
      }
      if (curIsMultidraw)
        multidrawElems.push_back({&re, curDynVarsState});
      else
        immediateElems.push_back(&re);
    }
  }

  // immediate draw
  {
    ShaderElement *curSElem = nullptr;
    Sbuffer *curVb = nullptr, *curIb = nullptr;
    for (const RElem *re : immediateElems)
    {
      if (re->shElem != curSElem)
      {
        curSElem = re->shElem;
        if (!re->shElem->setStates())
          G_ASSERT_CONTINUE(0);
      }
      if (curVb != re->vb)
      {
        curVb = re->vb;
        d3d_err(d3d::setvsrc(0, re->vb, re->vStride));
      }
      if (curIb != re->ib)
      {
        curIb = re->ib;
        d3d_err(d3d::setind(re->ib));
      }
      const uint32_t immOfs = re->instId * 4;
      d3d::set_immediate_const(STAGE_VS, &immOfs, 1);
      d3d::drawind(PRIM_TRILIST, re->ibOfs, re->fCnt, re->vbOfs);
    }
  }

  // multidraw
  if (!multidrawElems.empty())
  {
    for (const auto &[cstate, texLvl] : bindlessTexLevels)
      update_bindless_state(cstate, texLvl);

    const auto multiDrawRenderer = get_render_ctx().riMultidrawContext.fillBuffers(multidrawElems.size(),
      [&](uint32_t draw_index, uint32_t &index_count_per_instance, uint32_t &instance_count, uint32_t &start_index_location,
        int32_t &base_vertex_location, RiMultidrawPerInstData &per_draw_data) {
        const MultidrawElem &md = multidrawElems[draw_index];
        index_count_per_instance = md.re->fCnt * 3;
        instance_count = 1;
        start_index_location = md.re->ibOfs;
        base_vertex_location = md.re->vbOfs;
        const uint32_t instanceOffset = md.re->instId;
        G_LOGERR_ONCE_AND_DO(instanceOffset < MAX_MATRIX_OFFSET, instance_count = 0, "Too big offset in instance matrix buffer %d.",
          instanceOffset);
        const uint32_t materialOffset = get_material_offset(md.dynVarsState.const_state);
        G_LOGERR_ONCE_AND_DO(materialOffset < MAX_MATERIAL_OFFSET, instance_count = 0, "Too big material offset %d.", materialOffset);
        per_draw_data = (instanceOffset << MATRICES_OFFSET_SHIFT) | materialOffset;
      });

    ShaderElement *curSElem = nullptr;
    Sbuffer *curVb = nullptr, *curIb = nullptr;
    uint32_t flushFrom = 0;
    const auto flush = [&](uint32_t to_idx) {
      if (to_idx != flushFrom)
        multiDrawRenderer.render(PRIM_TRILIST, flushFrom, to_idx - flushFrom);
      flushFrom = to_idx;
    };
    for (uint32_t i = 0; i < multidrawElems.size(); i++)
    {
      const MultidrawElem &md = multidrawElems[i];
      const auto &re = *md.re;
      if (curSElem != re.shElem)
      {
        flush(i);
        set_states_for_variant(re.shElem->native(), md.dynVarsState.variant, md.dynVarsState.program, md.dynVarsState.state_index);
        curSElem = re.shElem;
      }
      if (curVb != re.vb)
      {
        flush(i);
        curVb = re.vb;
        d3d_err(d3d::setvsrc(0, re.vb, re.vStride));
      }
      if (curIb != re.ib)
      {
        flush(i);
        curIb = re.ib;
        d3d_err(d3d::setind(re.ib));
      }
    }
    flush(multidrawElems.size());
  }

  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, nullptr);
  rendinst::render::endRenderInstancing();
}

} // namespace frx
