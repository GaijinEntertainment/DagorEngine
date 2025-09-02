// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/nodeBasedShader.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_rwResource.h>
#include "render/enviCover/enviCover.h"
#include <ioSys/dag_dataBlock.h>


static int envi_cover_rw_gbuffer_slot = 4; // Must match the one in gbuffer_RW_init_DNG.hlsl

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
    enviCoverNBS->closeShader();

  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS->closeShader();
}

void EnviCover::initShader(const String &root_graph)
{
  closeShaders();

  enviCoverNBS = create_node_based_shader(NodeBasedShaderEnviCoverVariant::ENVI_COVER);
  enviCoverNBS->init(root_graph);

  enviCoverCombinedNBS = create_node_based_shader(NodeBasedShaderEnviCoverVariant::ENVI_COVER_WITH_MOTION_VECS);
  enviCoverCombinedNBS->init(root_graph);
}

bool EnviCover::updateShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  if (!enviCoverNBS->update(shader_name, shader_blk, out_errors))
    return false;

  if (!enviCoverCombinedNBS->update(shader_name, shader_blk, out_errors))
    return false;

  return true;
}

void EnviCover::initRender(EnviCoverUseType usage_type)
{
  usage = usage_type;

  if (usage == EnviCover::EnviCoverUseType::STANDALONE_FGNODE && !enviCoverStandaloneCS)
    enviCoverStandaloneCS.reset(new_compute_shader("envi_cover_cs"));

  if (usage == EnviCover::EnviCoverUseType::COMBINED_FGNODE && !enviCoverCombinedCS)
    enviCoverCombinedCS.reset(new_compute_shader("motion_vec_resolve_and_envi_cover_cs"));
}

void EnviCover::render(int x, int y, const eastl::array<BaseTexture *, ENVI_COVER_RW_TARGETS> &gbufBaseTextures)
{
  static int frame_idx = 0;

  ShaderGlobal::set_int(var::envi_cover_frame_idx, frame_idx++ % 4);

  for (int i = 0; i < ENVI_COVER_RW_TARGETS; i++)
  {
    d3d::set_rwtex(STAGE_CS, envi_cover_rw_gbuffer_slot + i, gbufBaseTextures[i], 0, 0); // State guards doesnt work with arrays
                                                                                         // because const & to element
  }
  switch (usage)
  {
    case EnviCover::EnviCoverUseType::STANDALONE_FGNODE: enviCoverStandaloneCS->dispatchThreads(x, y, 1); break;
    case EnviCover::EnviCoverUseType::COMBINED_FGNODE: enviCoverCombinedCS->dispatchThreads(x, y, 1); break;
    case EnviCover::EnviCoverUseType::NBS: enviCoverNBS->dispatch((x + 7) / 8, (y + 7) / 8, 1); break;
    case EnviCover::EnviCoverUseType::NBS_COMBINED: enviCoverCombinedNBS->dispatch((x + 7) / 8, (y + 7) / 8, 1); break;
    default: logerr("EnviCover::render was called without setting up initRender with a proper usage type!"); break;
  }
  for (int i = 0; i < ENVI_COVER_RW_TARGETS; i++)
  {
    if (gbufBaseTextures[i])
    {
      d3d::resource_barrier({gbufBaseTextures[i], RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0});
      d3d::set_rwtex(STAGE_CS, envi_cover_rw_gbuffer_slot + i, nullptr, 0, 0); // State guards doesnt work with arrays because const &
                                                                               // to element
    }
  }
}

void EnviCover::beforeReset() { closeShaders(); }

void EnviCover::afterReset()
{
  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS->reset();
  if (enviCoverNBS)
    enviCoverNBS->reset();
}

void EnviCover::enableOptionalShader(const String &shader_name, bool enable)
{
  if (enviCoverCombinedNBS)
    enviCoverCombinedNBS->enableOptionalGraph(shader_name, enable);
  if (enviCoverNBS)
    enviCoverNBS->enableOptionalGraph(shader_name, enable);
}

EnviCover::EnviCover() {}
EnviCover::~EnviCover() { closeShaders(); }