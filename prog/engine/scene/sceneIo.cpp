// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <scene/dag_scene.h>
#include <scene/dag_tiledScene.h>
#include <ioSys/dag_genIo.h>

static const int version = 1;
static const int magic_number = MAKE4C('S', 'S', 'C', 'N');

void scene::save_simple_scene(IGenSave &cb, const SimpleScene &scene)
{
  cb.beginTaggedBlock(magic_number);
  cb.writeInt(version);

  cb.beginBlock();
  cb.writeInt(scene.getPoolsCount());
  for (int i = 0; i < scene.getPoolsCount(); ++i)
  {
    auto &pool = scene.getPoolBbox(i);
    cb.write(&pool, sizeof(pool));
  }
  cb.endBlock();

  cb.beginBlock();
  cb.writeInt(scene.getNodesAliveCount());
  for (auto ni : scene)
  {
    auto n = scene.getNode(ni);
    cb.write(&n, sizeof(n));
  }
  cb.endBlock();

  cb.endBlock();
}

template <typename Scene>
bool scene::load_scene(IGenLoad &cb, Scene &scene)
{
  if (cb.beginTaggedBlock() != magic_number || cb.readInt() != version)
  {
    cb.endBlock();
    return false;
  }

  cb.beginBlock();
  const int poolsCount = cb.readInt();
  for (int i = 0; i < poolsCount; ++i)
  {
    bbox3f box;
    cb.read(&box, sizeof(box));
    scene.setPoolBBox(i, box);
    scene.setPoolDistanceSqScale(i, v_extract_w(box.bmin));
  }
  cb.endBlock();

  cb.beginBlock();
  const int nodesCount = cb.readInt();
  scene.reserve(nodesCount);

  for (int i = 0; i < nodesCount; ++i)
  {
    mat44f node;
    cb.read(&node, sizeof(node));
    scene.allocate(node, get_node_pool(node), get_node_flags(node));
  }
  cb.endBlock();

  cb.endBlock();
  return true;
}

template bool scene::load_scene(IGenLoad &cb, scene::SimpleScene &scene);

template bool scene::load_scene(IGenLoad &cb, scene::TiledScene &scene);
