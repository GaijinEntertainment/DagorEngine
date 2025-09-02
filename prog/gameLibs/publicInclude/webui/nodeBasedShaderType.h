//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum class NodeBasedShaderType
{
  Fog,
  EnviCover,
};

enum class NodeBasedShaderFogVariant : uint32_t
{
  Froxel,
  Raymarch,
  Shadow,
  COUNT,
};

enum class NodeBasedShaderEnviCoverVariant : uint32_t
{
  ENVI_COVER,
  ENVI_COVER_WITH_MOTION_VECS,
  COUNT,
};

inline String get_shader_name(NodeBasedShaderType shaderType)
{
  switch (shaderType)
  {
    case NodeBasedShaderType::Fog: return String("node_based_volumetric_fog_cs");
    case NodeBasedShaderType::EnviCover: return String("node_based_envi_cover_cs");
    default: G_ASSERTF(false, "Wrong shader type!"); return String();
  }
}

inline String get_shader_suffix(NodeBasedShaderType shaderType)
{
  switch (shaderType)
  {
    case NodeBasedShaderType::Fog: return String("volfog");
    case NodeBasedShaderType::EnviCover: return String("envi_cover");
    default: G_ASSERTF(false, "Wrong shader type!"); return String();
  }
}

inline uint32_t get_shader_variant_count(NodeBasedShaderType shaderType)
{
  switch (shaderType)
  {
    case NodeBasedShaderType::Fog: return (uint32_t)NodeBasedShaderFogVariant::COUNT;
    case NodeBasedShaderType::EnviCover: return (uint32_t)NodeBasedShaderEnviCoverVariant::COUNT;
    default: G_ASSERTF(false, "Wrong shader type!"); return 1;
  }
}
