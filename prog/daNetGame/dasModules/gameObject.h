// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_typedecl.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasModulesCommon.h>
#include "main/gameObjects.h"

MAKE_TYPE_FACTORY(GameObjects, GameObjects);
MAKE_TYPE_FACTORY(TiledScene, scene::TiledScene);

namespace bind_dascript
{
inline bool das_find_scene_game_objects(const scene::TiledScene *scene,
  const das::TBlock<bool, uint32_t, const das::TTemporary<const TMatrix>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  if (!scene)
    return false;
  vec4f args[2];
  bool found = false;
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      for (scene::node_index ni : *scene)
      {
        args[0] = das::cast<uint32_t>::from(ni);
        TMatrix mat;
        v_mat_43cu_from_mat44(mat.m[0], scene->getNode(ni));
        args[1] = das::cast<const TMatrix &>::from(mat);
        found = code->evalBool(*context);
        if (found)
          break;
      }
    },
    at);
  return found;
}

inline void das_for_scene_game_objects_in_box(const scene::TiledScene *scene,
  const BBox3 &culling_box,
  const das::TBlock<void, uint32_t, const das::TTemporary<const TMatrix>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  if (!scene)
    return;
  Point3_vec4 minPoint(culling_box.lim[0]);
  Point3_vec4 maxPoint(culling_box.lim[1]);
  bbox3f box;
  box.bmin = v_ldu(&minPoint.x);
  box.bmax = v_ldu(&maxPoint.x);
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      scene->boxCull<false, true>(box, 0, 0, [&](scene::node_index ni, const mat44f &m) {
        args[0] = das::cast<uint32_t>::from(ni);
        TMatrix mat;
        v_mat_43cu_from_mat44(mat.m[0], m);
        args[1] = das::cast<const TMatrix &>::from(mat);
        code->eval(*context);
      });
    },
    at);
}

inline void das_enum_sounds_in_box(const BBox3 &culling_box,
  uint32_t sound_type,
  const das::TBlock<void, uint32_t /*sound_shape*/, Point3 /*extents*/, const das::TTemporary<const TMatrix>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  const GameObjects *game_objects = ECS_GET_SINGLETON_COMPONENT(GameObjects, game_objects);
  auto scene = game_objects ? game_objects->sounds : nullptr;
  if (!scene)
    return;
  Point3_vec4 minPoint(culling_box.lim[0]);
  Point3_vec4 maxPoint(culling_box.lim[1]);
  bbox3f box;
  box.bmin = v_ldu(&minPoint.x);
  box.bmax = v_ldu(&maxPoint.x);
  vec4f args[4];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      scene->boxCull<false, true>(box, 0, 0, [&](scene::node_index /*ni*/, mat44f_cref m) {
        const GameObjectSoundAttributes::id_t id = scene::get_node_flags(m);
        if (const GameObjectSoundAttributes::Attributes *a = game_objects->soundAttributes.get(id))
          if (a->soundTypeHash == sound_type)
          {
            TMatrix mat;
            v_mat_43cu_from_mat44(mat.m[0], m);
            args[0] = das::cast<uint32_t>::from(a->soundShapeHash);
            args[1] = das::cast<Point3>::from(a->extents);
            args[2] = das::cast<const TMatrix &>::from(mat);
            code->eval(*context);
          }
      });
    },
    at);
}

inline const scene::TiledScene *das_get_scene_game_objects_by_name(const GameObjects &game_objects, const char *name)
{
  auto instances = game_objects.objects.find_as(name);
  if (instances != game_objects.objects.end())
    return instances->second.get();
  return nullptr;
}

inline Point3 das_game_objects_scene_get_node_pos(const scene::TiledScene &scene, int index)
{
  Point3_vec4 ret;
  v_st(&ret.x, scene.getNode(index).col3);
  return ret;
}

inline void das_for_scene_game_object_types(
  const GameObjects &game_objects, const das::TBlock<void, const char *> &block, das::Context *context, das::LineInfoArg *at)
{
  for (auto &pair : game_objects.objects)
  {
    vec4f arg = das::cast<const char *>::from(pair.first.c_str());
    context->invoke(block, &arg, nullptr, at);
  }
}

inline bool das_find_ladder(const Point3 &from, const Point3 &to, TMatrix &near_ladder, int &num_steps)
{
  bool near = false;
  traceray_ladders(from, to, [&](const TMatrix &tm, int numSteps) {
    near_ladder = tm;
    num_steps = numSteps;
    near = true;
  });
  return near;
}

inline uint32_t tiled_scene_getNodesCount(const scene::TiledScene &scene) { return scene.getNodesCount(); };
inline const mat44f &tiled_scene_getNode(const scene::TiledScene &scene, scene::node_index node) { return scene.getNode(node); };
inline scene::node_index tiled_scene_getNodeFromIndex(const scene::TiledScene &scene, uint32_t idx)
{
  return scene.getNodeFromIndex(idx);
};
inline uint32_t tiled_scene_getNodesAliveCount(const scene::TiledScene &scene) { return scene.getNodesAliveCount(); };
inline void tiled_scene_iterate(const scene::TiledScene &scene,
  const das::TBlock<void, const scene::node_index> &node_callback,
  das::Context *context,
  das::LineInfoArg *at)
{
  vec4f arg;
  context->invokeEx(
    node_callback, &arg, nullptr,
    [&](das::SimNode *code) {
      for (scene::node_index ni : scene)
      {
        arg = das::cast<const scene::node_index>::from(ni);
        code->eval(*context);
      }
    },
    at);
}

inline void tiled_scene_boxCull(const scene::TiledScene &scene,
  bbox3f_cref box,
  uint32_t test_flags,
  uint32_t equal_flags,
  const das::TBlock<void, const scene::node_index, const mat44f> &visible_nodes,
  das::Context *context,
  das::LineInfoArg *at)
{
  vec4f args[2];
  context->invokeEx(
    visible_nodes, args, nullptr,
    [&](das::SimNode *code) {
      scene.boxCull<false, true>(box, test_flags, equal_flags, [&](scene::node_index i, mat44f_cref m) {
        args[0] = das::cast<const scene::node_index>::from(i);
        args[1] = das::cast<const mat44f &>::from(m);
        code->eval(*context);
      });
    },
    at);
}
} // namespace bind_dascript
