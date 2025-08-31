//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_computeShaders.h>

/**
 * \brief Mesh shader wrapper for scripted shader.
 */
class MeshShaderElement : public ComputeShaderElement
{
public:
  DAG_DECLARE_NEW(midmem)
  decl_class_name(MeshShaderElement)

  MeshShaderElement(ScriptedShaderMaterial *ssm, ShaderElement *selem = nullptr) : ComputeShaderElement(ssm, selem) {}

  bool dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z, bool set_states = true) const;
  bool dispatchThreads(uint32_t threads_x, uint32_t threads_y, uint32_t threads_z, bool set_states = true) const;
  bool dispatchIndirect(Sbuffer *args, uint32_t count, uint32_t offset, uint32_t stride, bool set_states = true) const;
  bool dispatchIndirectCount(Sbuffer *args, uint32_t args_offset, uint32_t args_stride, Sbuffer *count, uint32_t count_offset,
    uint32_t max_count, bool set_states = true) const;
};

MeshShaderElement *new_mesh_shader(const char *shader_name, bool optional = false);

/**
 * \brief RAII wrapper for mesh shader.
 */
class MeshShader
{
  eastl::unique_ptr<MeshShaderElement> elem;

public:
  MeshShader() = default;
  MeshShader(const char *shader_name) : elem(shader_name ? new_mesh_shader(shader_name) : nullptr) {}
  bool dispatchGroups(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z) const
  {
    return elem->dispatch(groups_x, groups_y, groups_z);
  }
  bool dispatchThreads(uint32_t threads_x, uint32_t threads_y, uint32_t threads_z) const
  {
    return elem->dispatchThreads(threads_x, threads_y, threads_z);
  }
  bool dispatchIndirect(Sbuffer *args, uint32_t count, uint32_t offset, uint32_t stride) const
  {
    return elem->dispatchIndirect(args, count, offset, stride);
  }
  bool dispatchIndirectCount(Sbuffer *args, uint32_t args_offset, uint32_t args_stride, Sbuffer *count, uint32_t count_offset,
    uint32_t max_count) const
  {
    return elem->dispatchIndirectCount(args, args_offset, args_stride, count, count_offset, max_count);
  }
};
