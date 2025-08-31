// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/indoorProbeScenes.h>
#include "scene/dag_tiledScene.h"
#include <shaders/dag_shaders.h>
#include <daECS/core/ecsHash.h>
#include <math/dag_hlsl_floatx.h>
#include <util/dag_convar.h>
#include <render/indoor_probes_const.hlsli>

bool IndoorProbeScenes::addScene(const char *scene_name, eastl::unique_ptr<scene::TiledScene> &&scene, uint8_t shape_type)
{
  // Scene with that name already exists
  uint32_t nhash = ecs_str_hash(scene_name);
  auto foundScene = findScene([&](auto &d) { return d.nameHash == nhash; });
  if (foundScene != nullptr)
    return false;

  LightProbeSceneData data{};
  data.nameHash = nhash;
  data.name = scene_name;
  data.shapeType = shape_type;
  data.scenePtr = eastl::move(scene);

  for (const scene::node_index &nodeIdx : *data.scenePtr)
  {
    data.nodeIdxToProbeIdx[nodeIdx] = nodeCount;
    ++nodeCount;
  }
  data.rangeEnd = nodeCount;

  sceneDatas.emplace_back(eastl::move(data));
  return true;
}

const LightProbeSceneData *IndoorProbeScenes::getSceneDataForProbeIdx(int probeIdx)
{
  auto comp = [&probeIdx](LightProbeSceneData &data) { return probeIdx < data.rangeEnd; };

  auto shapeScene = findScene(comp);

  if (shapeScene == nullptr)
  {
    LOGWARN_ONCE("getSceneDataforProbeIdx: probeIdx out of range: %d", probeIdx);
    return nullptr;
  }

  return shapeScene;
}