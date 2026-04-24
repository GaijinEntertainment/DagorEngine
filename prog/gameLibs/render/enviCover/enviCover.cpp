// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/nodeBasedShader.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_rwResource.h>
#include "render/enviCover/enviCover.h"
#include <render/enviCover/enviCoverThread.hlsli>
#include <ioSys/dag_dataBlock.h>


static int envi_cover_rw_gbuffer_slot = 3; // Must match the one in gbuffer_rw_init_DNG.hlsl

namespace var
{
static ShaderVariableInfo envi_cover_frame_idx("envi_cover_frame_idx");
}

static eastl::unique_ptr<NodeBasedShader> create_node_based_shader(NodeBasedShaderEnviCoverVariant variant)
{
  return eastl::move(
    eastl::make_unique<NodeBasedShader>(NodeBasedShaderType::EnviCover, String(::get_shader_name(NodeBasedShaderType::EnviCover)),
      String(::get_shader_suffix(NodeBasedShaderType::EnviCover)), static_cast<uint32_t>(variant)));
}

void EnviCover::closeShaders()
{
  if (enviCoverNBS)
    enviCoverNBS.reset();

  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS.reset();

  if (enviCoverWithPackedNormsNBS)
    enviCoverWithPackedNormsNBS.reset();

  if (enviCoverCombinedWithPackedNormsNBS)
    enviCoverCombinedWithPackedNormsNBS.reset();
}

void EnviCover::initShader(const String &root_graph)
{
  closeShaders();

  enviCoverNBS = create_node_based_shader(NodeBasedShaderEnviCoverVariant::ENVI_COVER);
  enviCoverNBS->init(root_graph);

  enviCoverCombinedNBS = create_node_based_shader(NodeBasedShaderEnviCoverVariant::ENVI_COVER_COMBINED);
  enviCoverCombinedNBS->init(root_graph);

  enviCoverWithPackedNormsNBS = create_node_based_shader(NodeBasedShaderEnviCoverVariant::ENVI_COVER_WITH_NORMS_PACKED);
  enviCoverWithPackedNormsNBS->init(root_graph);

  enviCoverCombinedWithPackedNormsNBS =
    create_node_based_shader(NodeBasedShaderEnviCoverVariant::ENVI_COVER_COMBINED_WITH_NORMS_PACKED);
  enviCoverCombinedWithPackedNormsNBS->init(root_graph);
}

bool EnviCover::updateShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  if (!enviCoverNBS->update(shader_name, shader_blk, out_errors))
    return false;

  if (!enviCoverCombinedNBS->update(shader_name, shader_blk, out_errors))
    return false;

  if (!enviCoverWithPackedNormsNBS->update(shader_name, shader_blk, out_errors))
    return false;

  if (!enviCoverCombinedWithPackedNormsNBS->update(shader_name, shader_blk, out_errors))
    return false;

  return true;
}

void EnviCover::initRender(EnviCoverUseType envi_cover_type)
{
  usage = envi_cover_type;
  if (usage == EnviCoverUseType::COMBINED_FGNODE)
  {
    if (!enviCoverCombinedCS)
      enviCoverCombinedCS.reset(new_compute_shader("motion_vec_resolve_and_envi_cover_cs"));
  }
  else if (usage == EnviCoverUseType::STANDALONE_FGNODE)
  {
    if (!enviCoverStandaloneCS)
      enviCoverStandaloneCS.reset(new_compute_shader("envi_cover_cs"));
  }
}

void EnviCover::render(int x, int y, const eastl::array<BaseTexture *, ENVI_COVER_MAX_RW_TARGETS> &&gbufBaseTextures)
{
  static int frame_idx = 0;

  ShaderGlobal::set_int(var::envi_cover_frame_idx, frame_idx++ % 4);

  for (int i = 0; i < ENVI_COVER_MAX_RW_TARGETS; i++)
  {
    d3d::set_rwtex(STAGE_CS, envi_cover_rw_gbuffer_slot + i, gbufBaseTextures[i], 0, 0);
  }

  const int threadX = (x + ENVI_COVER_THREADS_X - 1) / ENVI_COVER_THREADS_X;
  const int threadY = (y + ENVI_COVER_THREADS_Y - 1) / ENVI_COVER_THREADS_Y;
  const int threadZ = ENVI_COVER_THREADS_Z;

  switch (usage)
  {
    case EnviCoverUseType::STANDALONE_FGNODE: enviCoverStandaloneCS->dispatch(threadX, threadY, threadZ); break;
    case EnviCoverUseType::COMBINED_FGNODE: enviCoverCombinedCS->dispatch(threadX, threadY, threadZ); break;
    case EnviCoverUseType::NBS: enviCoverNBS->dispatch(threadX, threadY, threadZ); break;
    case EnviCoverUseType::NBS_COMBINED: enviCoverCombinedNBS->dispatch(threadX, threadY, threadZ); break;
    case EnviCoverUseType::NBS_PACKED_NORMS: enviCoverWithPackedNormsNBS->dispatch(threadX, threadY, threadZ); break;
    case EnviCoverUseType::NBS_COMBINED_PACKED_NORMS: enviCoverCombinedWithPackedNormsNBS->dispatch(threadX, threadY, threadZ); break;
    default: logerr("EnviCover::render was called without setting up initRender with a proper usage type!"); break;
  }
  for (int i = 0; i < ENVI_COVER_MAX_RW_TARGETS; i++)
  {
    if (gbufBaseTextures[i])
    {
      d3d::resource_barrier({gbufBaseTextures[i], RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0});
      d3d::set_rwtex(STAGE_CS, envi_cover_rw_gbuffer_slot + i, nullptr, 0, 0); // State guards doesnt work with arrays because const &
                                                                               // to element
    }
  }
}

void EnviCover::beforeReset()
{
  if (enviCoverNBS)
    enviCoverNBS->closeShader();

  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS->closeShader();

  if (enviCoverWithPackedNormsNBS)
    enviCoverWithPackedNormsNBS->closeShader();

  if (enviCoverCombinedWithPackedNormsNBS)
    enviCoverCombinedWithPackedNormsNBS->closeShader();
}

void EnviCover::afterReset()
{
  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS->reset();

  if (enviCoverNBS)
    enviCoverNBS->reset();

  if (enviCoverWithPackedNormsNBS)
    enviCoverWithPackedNormsNBS->reset();

  if (enviCoverCombinedWithPackedNormsNBS)
    enviCoverCombinedWithPackedNormsNBS->reset();
}

void EnviCover::setNbsQuality(NodeBasedShaderQuality nbs_quality)
{
  if (enviCoverNBS)
    enviCoverNBS->setQuality(nbs_quality);

  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS->setQuality(nbs_quality);

  if (enviCoverWithPackedNormsNBS)
    enviCoverWithPackedNormsNBS->setQuality(nbs_quality);

  if (enviCoverCombinedWithPackedNormsNBS)
    enviCoverCombinedWithPackedNormsNBS->setQuality(nbs_quality);
}

void EnviCover::enableOptionalShader(const String &shader_name, bool enable)
{
  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS->enableOptionalGraph(shader_name, enable);

  if (enviCoverNBS)
    enviCoverNBS->enableOptionalGraph(shader_name, enable);

  if (enviCoverWithPackedNormsNBS)
    enviCoverWithPackedNormsNBS->enableOptionalGraph(shader_name, enable);

  if (enviCoverCombinedWithPackedNormsNBS)
    enviCoverCombinedWithPackedNormsNBS->enableOptionalGraph(shader_name, enable);
}

EnviCover::EnviCover() {}
EnviCover::~EnviCover() { closeShaders(); }
