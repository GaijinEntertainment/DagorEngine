//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/baseComponentTypes/objectType.h>
#include <EASTL/vector.h>
#include <render/daFrameGraph/daFG.h>


struct TriangleSizeSystems
{
  bool isRI = false;
  bool isTree = false;
  bool isDaGDP = false;
  bool isGround = false;
  bool isDynamics = false;
  bool isTransparent = false;

  bool isAnyActive() const { return isRI || isTree || isDaGDP || isGround || isDynamics || isTransparent; }
};

struct CreateTriangleDebugNodes : public ecs::Event
{
  eastl::vector<dafg::NodeHandle> *nodes;
  const TriangleSizeSystems &systems;
  ECS_BROADCAST_EVENT_DECL(CreateTriangleDebugNodes)
  CreateTriangleDebugNodes(eastl::vector<dafg::NodeHandle> *nodes, const TriangleSizeSystems &systems) :
    ECS_EVENT_CONSTRUCTOR(CreateTriangleDebugNodes), nodes(nodes), systems(systems)
  {}

  dafg::NameSpace getNameSpace() const { return (dafg::root() / "tringle_size_debug"); }
};


struct DestroyTriangleDebugNodes : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(DestroyTriangleDebugNodes)
  DestroyTriangleDebugNodes() : ECS_EVENT_CONSTRUCTOR(DestroyTriangleDebugNodes) {}
};