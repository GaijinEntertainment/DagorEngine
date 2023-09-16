#pragma once

enum class ShaderType : int
{
  Invalid = -1,
  Vertex = 1,
  Pixel = 2,
  Compute = 3
};

inline ShaderType profile_to_shader_type(const char *profile)
{
  switch (profile[0])
  {
    case 'c': return ShaderType::Compute;
    case 'v': return ShaderType::Vertex;
    case 'p': return ShaderType::Pixel;
    default: return ShaderType::Invalid;
  }
  return ShaderType::Invalid;
}
