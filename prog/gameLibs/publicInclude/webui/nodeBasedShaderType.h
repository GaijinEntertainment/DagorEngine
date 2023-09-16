//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

enum class NodeBasedShaderType
{
  Fog,
};

enum class NodeBasedShaderFogVariant : uint32_t
{
  Froxel,
  Raymarch,
  Shadow,
  COUNT,
};

inline String get_shader_name(NodeBasedShaderType shaderType)
{
  switch (shaderType)
  {
    case NodeBasedShaderType::Fog: return String("node_based_volumetric_fog_cs");
    default: G_ASSERTF(false, "Wrong shader type!"); return String();
  }
}

inline String get_shader_suffix(NodeBasedShaderType shaderType)
{
  switch (shaderType)
  {
    case NodeBasedShaderType::Fog: return String("volfog");
    default: G_ASSERTF(false, "Wrong shader type!"); return String();
  }
}

inline uint32_t get_shader_variant_count(NodeBasedShaderType shaderType)
{
  switch (shaderType)
  {
    case NodeBasedShaderType::Fog: return (uint32_t)NodeBasedShaderFogVariant::COUNT;
    default: G_ASSERTF(false, "Wrong shader type!"); return 1;
  }
}
