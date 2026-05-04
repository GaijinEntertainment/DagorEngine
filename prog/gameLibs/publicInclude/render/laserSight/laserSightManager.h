//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "laser_sight.hlsli"
#include <projectiveDecals/projectiveDecals.h>
#include <daECS/core/componentType.h>


struct LaserDecalData : public DecalDataBase
{
  float3 right;
  uint is_from_fps_view; // 7 bits texture id, 9 bit matrix id, 16 bits flags
  float4 color;
  float3 origin_pos;
  uint pad2;

  LaserDecalData(uint32_t decal_id, float rad, bool is_from_fps_view, const Point3 &normal, const Point3 &right, const Point3 &pos,
    const Point4 &color, const Point3 &origin_pos);
  LaserDecalData();
  LaserDecalData(uint32_t id);

  void partialUpdate(dag::Span<Point4>) const { G_ASSERT(0); }
};

using LaserDecalManager = ResizableDecalsManagerBase<LaserDecalInstance, LaserDecalData>;

ECS_DECLARE_RELOCATABLE_TYPE(LaserDecalManager);