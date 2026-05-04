// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include "bvh_context.h"
#include "bvh_tools.h"
#include <dag/dag_vector.h>
#include <shaders/dag_renderScene.h>
#include <scene/dag_renderSceneMgr.h>
#include <streaming/dag_streamingBase.h>

namespace bvh::binscene
{

void init() {}

void teardown() {}

static void add_mesh(ContextId context_id, uint64_t mesh_id, const MeshInfo &info)
{
  ObjectInfo obj;
  obj.meshes.push_back(info);
  obj.tag = "binscene";
  add_object(context_id, mesh_id, obj);
}

struct ElemCallback : public RenderScene::ElemCallback
{
  bvh::ContextId bvhCtx;

  void on_elem_found(const Data &data) override
  {
    G_ASSERT(data.normalFormat == -1 || data.normalFormat == VSDT_E3DCOLOR);
    G_ASSERT(data.colorFormat == -1 || data.colorFormat == VSDT_E3DCOLOR);

    uint64_t id = uint64_t(bvh_id_gen.fetch_add(1)) << 32 | uint64_t(1) << 63;
    bvhCtx->binSceneObjectIds.push_back(id);

    bvh::MeshInfo meshInfo;
    meshInfo.indices = data.indices;
    meshInfo.indexCount = data.indexCount;
    meshInfo.startIndex = data.startIndex;
    meshInfo.vertices = data.vertices;
    meshInfo.vertexCount = data.vertexCount;
    meshInfo.startVertex = data.startVertex;
    meshInfo.vertexSize = data.vertexStride;
    meshInfo.positionOffset = data.positionOffset;
    meshInfo.positionFormat = data.positionFormat;
    meshInfo.texcoordOffset = data.texcoordOffset;
    meshInfo.texcoordFormat = data.texcoordFormat;
    meshInfo.normalOffset = data.normalOffset;
    meshInfo.posMul = Point4::ONE;
    meshInfo.posAdd = Point4::ZERO;
    meshInfo.albedoTextureId = data.albedoTextureId;

    if (meshInfo.texcoordFormat == VSDT_SHORT2)
      meshInfo.texcoordFormat = bvh::BufferProcessor::bvhAttributeShort2TC;

    bvh::binscene::add_mesh(bvhCtx, id, meshInfo);
  }
} elemCallback;

void add_meshes(ContextId context_id, BaseStreamingSceneHolder &bin_scene)
{
  if (!context_id->has(Features::BinScene))
    return;

  elemCallback.bvhCtx = context_id;

  RenderSceneManager &sm = bin_scene.getRsm();
  for (RenderScene *scene : sm.getScenes())
  {
    if (!scene)
      continue;

    scene->foreachElem(elemCallback);
  }
}

void update_instances(ContextId context_id)
{
  if (!context_id->has(Features::BinScene))
    return;

  mat43f instanceTransform;
  instanceTransform.row0 = v_make_vec4f(1, 0, 0, 0);
  instanceTransform.row1 = v_make_vec4f(0, 1, 0, 0);
  instanceTransform.row2 = v_make_vec4f(0, 0, 1, 0);
  for (auto objectId : context_id->binSceneObjectIds)
    bvh::add_instance(context_id, objectId, instanceTransform);
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::BinScene))
    return;

  for (auto objectId : context_id->binSceneObjectIds)
    bvh::remove_object(context_id, objectId);
  context_id->binSceneObjectIds = {};
}
} // namespace bvh::binscene
