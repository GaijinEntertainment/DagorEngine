//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <ecs/game/generic/grid.h>
#include <math/dag_vecMathCompatibility.h>

DAS_BIND_ENUM_CAST(GridEntCheck);

MAKE_TYPE_FACTORY(GridHolder, GridHolder);

namespace bind_dascript
{
inline void _builtin_gather_entities_in_grid_box(uint32_t grid_name_hash, const BBox3 &bbox, GridEntCheck check_type,
  const das::TBlock<void, das::TTemporary<das::TArray<ecs::EntityId>>> &block, das::Context *context, das::LineInfoArg *at)
{
  TempGridEntities entities;
  gather_entities_in_grid(grid_name_hash, bbox, check_type, entities);

  das::Array arr;
  arr.data = (char *)entities.data();
  arr.size = entities.size();
  arr.capacity = entities.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void _builtin_gather_entities_in_grid_sphere(uint32_t grid_name_hash, const BSphere3 &bsphere, GridEntCheck check_type,
  const das::TBlock<void, das::TTemporary<das::TArray<ecs::EntityId>>> &block, das::Context *context, das::LineInfoArg *at)
{
  TempGridEntities entities;
  gather_entities_in_grid(grid_name_hash, bsphere, check_type, entities);

  das::Array arr;
  arr.data = (char *)entities.data();
  arr.size = entities.size();
  arr.capacity = entities.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void _builtin_gather_entities_in_grid_capsule(uint32_t grid_name_hash, const Point3 &from, const Point3 &dir, float len,
  float radius, GridEntCheck check_type, const das::TBlock<void, das::TTemporary<das::TArray<ecs::EntityId>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  TempGridEntities entities;
  gather_entities_in_grid(grid_name_hash, from, dir, len, radius, check_type, entities);

  das::Array arr;
  arr.data = (char *)entities.data();
  arr.size = entities.size();
  arr.capacity = entities.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void _builtin_gather_entities_in_grid_tm_box(uint32_t grid_name_hash, const TMatrix &transform, const BBox3 &bbox,
  GridEntCheck check_type, const das::TBlock<void, das::TTemporary<das::TArray<ecs::EntityId>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  TempGridEntities entities;
  gather_entities_in_grid(grid_name_hash, transform, bbox, check_type, entities);

  das::Array arr;
  arr.data = (char *)entities.data();
  arr.size = entities.size();
  arr.capacity = entities.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void _builtin_append_entities_in_grid_box(uint32_t grid_name_hash, const BBox3 &bbox, GridEntCheck check_type, das::Array &arr,
  das::Context *context, das::LineInfoArg *at)
{
  for_each_entity_in_grid(grid_name_hash, bbox, check_type, [&](ecs::EntityId eid, vec3f) {
    int idx = das::builtin_array_push_back(arr, 256, context, at);
    reinterpret_cast<ecs::EntityId *>(arr.data)[idx] = eid;
  });
}

inline void _builtin_append_entities_in_grid_sphere(uint32_t grid_name_hash, const BSphere3 &bsphere, GridEntCheck check_type,
  das::Array &arr, das::Context *context, das::LineInfoArg *at)
{
  for_each_entity_in_grid(grid_name_hash, bsphere, check_type, [&](ecs::EntityId eid, vec3f) {
    int idx = das::builtin_array_push_back(arr, 256, context, at);
    reinterpret_cast<ecs::EntityId *>(arr.data)[idx] = eid;
  });
}

inline void _builtin_append_entities_in_grid_capsule(uint32_t grid_name_hash, const Point3 &from, const Point3 &dir, float len,
  float radius, GridEntCheck check_type, das::Array &arr, das::Context *context, das::LineInfoArg *at)
{
  for_each_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type, [&](ecs::EntityId eid, vec3f) {
    int idx = das::builtin_array_push_back(arr, 256, context, at);
    reinterpret_cast<ecs::EntityId *>(arr.data)[idx] = eid;
  });
}

inline void _builtin_append_entities_in_grid_tm_box(uint32_t grid_name_hash, const TMatrix &transform, const BBox3 &bbox,
  GridEntCheck check_type, das::Array &arr, das::Context *context, das::LineInfoArg *at)
{
  for_each_entity_in_grid(grid_name_hash, transform, bbox, check_type, [&](ecs::EntityId eid, vec3f) {
    int idx = das::builtin_array_push_back(arr, 256, context, at);
    reinterpret_cast<ecs::EntityId *>(arr.data)[idx] = eid;
  });
}

inline ecs::EntityId _builtin_find_entity_in_grid_box(uint32_t grid_name_hash, const BBox3 &box, GridEntCheck check_type,
  const das::TBlock<bool, ecs::EntityId, das::float3> &block, das::Context *context, das::LineInfoArg *at)
{
  ecs::EntityId result = ecs::INVALID_ENTITY_ID;
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      result = find_entity_in_grid(grid_name_hash, box, check_type, [&](ecs::EntityId eid, vec3f pos) {
        args[0] = das::cast<ecs::EntityId>::from(eid);
        args[1] = das::cast<das::float3>::from(pos);
        return code->evalBool(*context);
      });
    },
    at);
  return result;
}

inline ecs::EntityId _builtin_find_entity_in_grid_sphere(uint32_t grid_name_hash, const BSphere3 &sphere, GridEntCheck check_type,
  const das::TBlock<bool, ecs::EntityId, das::float3> &block, das::Context *context, das::LineInfoArg *at)
{
  ecs::EntityId result = ecs::INVALID_ENTITY_ID;
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      result = find_entity_in_grid(grid_name_hash, sphere, check_type, [&](ecs::EntityId eid, vec3f pos) {
        args[0] = das::cast<ecs::EntityId>::from(eid);
        args[1] = das::cast<das::float3>::from(pos);
        return code->evalBool(*context);
      });
    },
    at);
  return result;
}

inline ecs::EntityId _builtin_find_entity_in_grid_capsule(uint32_t grid_name_hash, const Point3 &from, const Point3 &dir, float len,
  float radius, GridEntCheck check_type, const das::TBlock<bool, ecs::EntityId, das::float3> &block, das::Context *context,
  das::LineInfoArg *at)
{
  ecs::EntityId result = ecs::INVALID_ENTITY_ID;
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      result = find_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type, [&](ecs::EntityId eid, vec3f pos) {
        args[0] = das::cast<ecs::EntityId>::from(eid);
        args[1] = das::cast<das::float3>::from(pos);
        return code->evalBool(*context);
      });
    },
    at);
  return result;
}

inline ecs::EntityId _builtin_find_entity_in_grid_tm_box(uint32_t grid_name_hash, const TMatrix &transform, const BBox3 &box,
  GridEntCheck check_type, const das::TBlock<bool, ecs::EntityId, das::float3> &block, das::Context *context, das::LineInfoArg *at)
{
  ecs::EntityId result = ecs::INVALID_ENTITY_ID;
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      result = find_entity_in_grid(grid_name_hash, transform, box, check_type, [&](ecs::EntityId eid, vec3f pos) {
        args[0] = das::cast<ecs::EntityId>::from(eid);
        args[1] = das::cast<das::float3>::from(pos);
        return code->evalBool(*context);
      });
    },
    at);
  return result;
}
} // namespace bind_dascript
