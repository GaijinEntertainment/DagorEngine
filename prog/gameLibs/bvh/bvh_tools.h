// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderMatData.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_zip.h>
#include <atomic>

#include "bvh_context.h"
#include "bvh_ri_common.h"

#include <EASTL/fixed_vector.h>

namespace bvh
{

extern elem_rules_fn elem_rules;

extern dag::AtomicInteger<uint32_t> bvh_id_gen;

inline uint64_t make_relem_mesh_id(uint32_t res_id, uint32_t lod_id, uint32_t elem_id)
{
  return (uint64_t(res_id) << 32) | uint64_t(lod_id << 28) | elem_id; //-V629 //-V1028
}

inline void decompose_relem_mesh_id(uint64_t object_id, uint32_t &res_id, uint32_t &lod_id, uint32_t &elem_id)
{
  res_id = object_id >> 32;
  lod_id = (object_id >> 28) & ((1 << 4) - 1);
  elem_id = object_id & ((1 << 28) - 1);
}

inline uint32_t make_dyn_elem_id(uint32_t mesh_no, uint32_t elem_id) { return mesh_no << 14 | elem_id; }

inline eastl::optional<MeshInfo> process_relem(ContextId context_id, const ShaderMesh::RElem &elem,
  const BufferProcessor *vertex_processor, const Point4 &pos_mul, const Point4 &pos_add, const BSphere3 &bounding, bool is_ri,
  const RenderableInstanceLodsResource::ImpostorParams *impostor_params = nullptr,
  const RenderableInstanceLodsResource::ImpostorTextures *impostor_textures = nullptr)
{
  TIME_D3D_PROFILE(process_relem);

  G_UNUSED(impostor_params);

  static int use_cross_dissolveVarId = get_shader_variable_id("use_cross_dissolve", true);
  if (int use_cross_dissolve = 0; strcmp(elem.mat->getShaderClassName(), "rendinst_baked_impostor") == 0 &&
                                  elem.mat->getIntVariable(use_cross_dissolveVarId, use_cross_dissolve) && use_cross_dissolve)
    return eastl::nullopt;

  bool hasIndices;
  if (!elem.vertexData->isRenderable(hasIndices))
    return eastl::nullopt;

  ChannelParser parser;
  if (!elem.mat->enum_channels(parser, parser.flags))
    return eastl::nullopt;

  if (parser.positionFormat == -1)
    return eastl::nullopt;

  G_ASSERT(parser.normalFormat == -1 || parser.normalFormat == VSDT_E3DCOLOR);
  G_ASSERT(parser.colorFormat == -1 || parser.colorFormat == VSDT_E3DCOLOR);

  bool isHeliRotor = strncmp(elem.mat->getShaderClassName(), "helicopter_rotor", 16) == 0;
  bool isTree = strncmp(elem.mat->getShaderClassName(), "rendinst_tree", 13) == 0;
  bool isTreePerlinLayered = strncmp(elem.mat->getShaderClassName(), "rendinst_tree_perlin_layered", 28) == 0;
  bool isLeaves = strncmp(elem.mat->getShaderClassName(), "rendinst_facing_leaves", 22) == 0;
  bool isFlag = strncmp(elem.mat->getShaderClassName(), "rendinst_flag", 13) == 0;
  bool isDeformed = strcmp(elem.mat->getShaderClassName(), "dynamic_deformed") == 0;

  if (isHeliRotor)
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getHeliRotorVertexProcessor();
  }
  else if (isFlag)
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getFlagVertexProcessor();
  }
  else if (isDeformed)
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getDeformedVertexProcessor();
  }
  else if (impostor_textures && impostor_params)
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getImpostorVertexProcessor();
  }
  else if (isLeaves && context_id->has(Features::RIFull))
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getLeavesVertexProcessor();
  }
  else if (isTree && context_id->has(Features::RIFull))
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getTreeVertexProcessor();
  }
  else if (is_ri && context_id->has(Features::RIBaked))
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getBakeTextureToVerticesProcessor();
  }
  else if (!is_ri && !vertex_processor && context_id->has(Features::DynrendRigidBaked))
  {
    G_ASSERT(!vertex_processor);
    vertex_processor = &ProcessorInstances::getBakeTextureToVerticesProcessor();
  }

  auto ib = hasIndices ? elem.vertexData->getIB() : nullptr;
  auto vb = elem.vertexData->getVB();

  MeshInfo meshInfo{};
  meshInfo.indices = ib;
  meshInfo.indexCount = elem.numf * 3;
  meshInfo.startIndex = elem.si;
  meshInfo.vertices = vb;
  meshInfo.vertexCount = elem.numv;
  meshInfo.vertexSize = elem.vertexData->getStride();
  meshInfo.baseVertex = elem.baseVertex;
  meshInfo.startVertex = elem.sv;
  meshInfo.positionFormat = parser.positionFormat;
  meshInfo.positionOffset = parser.positionOffset;
  meshInfo.indicesOffset = parser.indicesFormat != -1 ? parser.indicesOffset : MeshInfo::invalidOffset;
  meshInfo.weightsOffset = parser.weightsFormat != -1 ? parser.weightsOffset : MeshInfo::invalidOffset;
  meshInfo.normalOffset = parser.normalFormat == VSDT_E3DCOLOR ? parser.normalOffset : MeshInfo::invalidOffset;
  meshInfo.colorOffset = parser.colorFormat == VSDT_E3DCOLOR ? parser.colorOffset : MeshInfo::invalidOffset;
  meshInfo.texcoordFormat = parser.texcoordFormat;
  meshInfo.texcoordOffset = parser.texcoordFormat != -1 ? parser.texcoordOffset : MeshInfo::invalidOffset;
  meshInfo.secTexcoordOffset = parser.secTexcoordFormat != -1 ? parser.secTexcoordOffset : MeshInfo::invalidOffset;
  meshInfo.vertexProcessor = vertex_processor;
  meshInfo.posMul = pos_mul;
  meshInfo.posAdd = pos_add;
  meshInfo.boundingSphere = bounding;
  meshInfo.hasInstanceColor = (isTree && !isTreePerlinLayered) || impostor_params;
  meshInfo.isHeliRotor = isHeliRotor;

  if (elem_rules)
    elem_rules(elem, meshInfo, parser, impostor_params, impostor_textures);

  return meshInfo;
}

inline void process_relems(ContextId context_id, const dag::Span<ShaderMesh::RElem> &elems, uint32_t bvh_id, uint32_t lod_ix,
  const BufferProcessor *vertex_processor, const Point4 &pos_mul, const Point4 &pos_add, const BSphere3 &bounding, bool is_ri,
  const RenderableInstanceLodsResource::ImpostorParams *impostor_params = nullptr,
  const RenderableInstanceLodsResource::ImpostorTextures *impostor_textures = nullptr)
{
  constexpr size_t MAX_RELEMS = 32;
  eastl::fixed_vector<MeshInfo, MAX_RELEMS> meshes;
  eastl::fixed_vector<int, MAX_RELEMS> indices;

  bool isAnimated = false;

  for (auto [i, elem] : enumerate(elems))
    if (auto meshInfo =
          process_relem(context_id, elem, vertex_processor, pos_mul, pos_add, bounding, is_ri, impostor_params, impostor_textures))
    {
      isAnimated = isAnimated || (meshInfo->vertexProcessor && !meshInfo->vertexProcessor->isOneTimeOnly());
      meshes.push_back(eastl::move(meshInfo.value()));
      indices.push_back(i);
    }

  if (meshes.empty())
    return;

  if (isAnimated)
  {
    for (auto [i, info] : zip(indices, meshes))
    {
      const auto meshId = make_relem_mesh_id(bvh_id, lod_ix, i);
      add_object(context_id, meshId, {{info}, isAnimated});
    }
  }
  else
  {
    const auto meshId = make_relem_mesh_id(bvh_id, lod_ix, 0);
    add_object(context_id, meshId, {{meshes.begin(), meshes.end()}, isAnimated});
  }
}

} // namespace bvh
