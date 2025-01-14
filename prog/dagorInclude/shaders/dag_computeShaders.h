//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_DObject.h>
#include <memory/dag_mem.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>

class ScriptedShaderMaterial;
class ScriptedShaderElement;
class Sbuffer;
class ShaderElement;


class ComputeShaderElement : public DObject
{
public:
  DAG_DECLARE_NEW(midmem)
  decl_class_name(ComputeShaderElement)

  ComputeShaderElement(ScriptedShaderMaterial *ssm, ShaderElement *selem = nullptr);
  ~ComputeShaderElement();

  const char *getShaderClassName() const;

  bool set_int_param(const int variable_id, const int v);
  bool set_real_param(const int variable_id, const real v);
  bool set_color4_param(const int variable_id, const struct Color4 &);
  bool set_texture_param(const int variable_id, const TEXTUREID v);
  bool set_sampler_param(const int variable_id, d3d::SamplerHandle v);

  bool hasVariable(const int variable_id) const;
  bool getColor4Variable(const int variable_id, Color4 &value) const;
  bool getRealVariable(const int variable_id, real &value) const;
  bool getIntVariable(const int variable_id, int &value) const;
  bool getTextureVariable(const int variable_id, TEXTUREID &value) const;
  bool getSamplerVariable(const int variable_id, d3d::SamplerHandle &value) const;

  // dispatch compute threads
  eastl::array<uint16_t, 3> getThreadGroupSizes() const;
  bool dispatch(int tgx, int tgy, int tgz, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const;
  bool dispatchThreads(int threads_x, int threads_y, int threads_z, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS,
    bool set_states = true) const;
  uint32_t getWaveSize() const;
  bool dispatch(GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const
  {
    return dispatch(1, 1, 1, gpu_pipeline, set_states);
  }
  bool dispatch_indirect(Sbuffer *args, int ofs, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const;
  bool setStates() const;

protected:
  ScriptedShaderMaterial *mat;
  ScriptedShaderElement *elem;
};


ComputeShaderElement *new_compute_shader(const char *shader_name, bool optional = false);

class ComputeShader
{
  eastl::unique_ptr<ComputeShaderElement> elem;

public:
  ComputeShader() = default;
  ComputeShader(const char *shader_name) : elem(shader_name ? new_compute_shader(shader_name) : nullptr) {}
  bool dispatchThreads(int threads_x, int threads_y, int threads_z) const
  {
    return elem->dispatchThreads(threads_x, threads_y, threads_z);
  }
  bool dispatchGroups(int groups_x, int groups_y, int groups_z) const { return elem->dispatch(groups_x, groups_y, groups_z); }
  bool dispatchIndirect(Sbuffer *args, int ofs) const { return elem->dispatch_indirect(args, ofs); }
};
