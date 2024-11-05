//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// clang-format off
template <typename FI> class StaticSceneRayTracerT;
template <typename FI> class DeserializedStaticSceneRayTracerT;
template <typename FI> class BuildableStaticSceneRayTracerT;
// clang-format on


union SceneRayI24F8
{
  struct
  {
    unsigned index : 24;
    unsigned flags : 8;
  };
  unsigned u32;
  operator int() const { return index; }
};

typedef StaticSceneRayTracerT<SceneRayI24F8> StaticSceneRayTracer;
typedef DeserializedStaticSceneRayTracerT<SceneRayI24F8> DeserializedStaticSceneRayTracer;
typedef BuildableStaticSceneRayTracerT<SceneRayI24F8> BuildableStaticSceneRayTracer;
