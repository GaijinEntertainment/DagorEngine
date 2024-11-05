//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <generic/dag_relocatableFixedVector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class String;

struct E3DCOLOR;
class Point3;

namespace scene
{
class TiledScene;
}

struct LightProbeSceneData
{
  uint32_t nameHash; // FNV1 hash of name
  int rangeEnd;
  eastl::unique_ptr<scene::TiledScene> scenePtr;
  ska::flat_hash_map<uint32_t, uint32_t> nodeIdxToProbeIdx;
  const char *name;
  uint8_t shapeType;
};
DAG_DECLARE_RELOCATABLE(LightProbeSceneData);

class IndoorProbeScenes
{
  uint32_t nodeCount = 0;

public:
  dag::RelocatableFixedVector<LightProbeSceneData, 1, /*overflow*/ true> sceneDatas;

  bool addScene(const char *scene_name, eastl::unique_ptr<scene::TiledScene> &&scene, uint8_t shape_type);

  uint32_t getAllNodesCount() const { return nodeCount; }

  const scene::TiledScene *getScene(uint32_t nhash);
  const LightProbeSceneData *getSceneDataForProbeIdx(int probeIdx);

private:
  template <typename F>
  inline const LightProbeSceneData *findScene(const F &cmp)
  {
    for (auto &sceneData : sceneDatas)
      if (cmp(sceneData))
        return &sceneData;
    return nullptr;
  }
};

inline const scene::TiledScene *IndoorProbeScenes::getScene(uint32_t nhash)
{
  auto shapeScene = findScene([&](auto &d) { return d.nameHash == nhash; });
  return shapeScene ? shapeScene->scenePtr.get() : nullptr;
}