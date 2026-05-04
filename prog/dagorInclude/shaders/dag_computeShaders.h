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


/// Compute shader element with full control over parameter binding and dispatch.
///
/// Created via new_compute_shader(). Wraps a ScriptedShaderMaterial and provides
/// methods to set shader parameters, query thread group sizes, and dispatch
/// compute work on the GPU.
class ComputeShaderElement : public DObject
{
public:
  DAG_DECLARE_NEW(midmem)
  decl_class_name(ComputeShaderElement)

  /// @param ssm Scripted shader material that defines this compute shader.
  /// @param selem Optional shader element override; if null, created from @p ssm.
  ComputeShaderElement(ScriptedShaderMaterial *ssm, ShaderElement *selem = nullptr);
  ~ComputeShaderElement();

  /// Returns the DSHL shader class name.
  const char *getShaderClassName() const;

  /// @name Parameter setters
  /// Set shader parameters by variable ID (obtained from get_shader_variable_id()).
  /// @return true if the variable was found and set successfully.
  /// @{
  bool set_int_param(const int variable_id, const int v);
  bool set_real_param(const int variable_id, const real v);
  bool set_color4_param(const int variable_id, const struct Color4 &);
  bool set_texture_param(const int variable_id, const TEXTUREID v);
  bool set_sampler_param(const int variable_id, d3d::SamplerHandle v);
  /// @}

  /// @name Parameter getters
  /// Query current parameter values. @return true if the variable exists.
  /// @{
  bool hasVariable(const int variable_id) const;
  bool getColor4Variable(const int variable_id, Color4 &value) const;
  bool getRealVariable(const int variable_id, real &value) const;
  bool getIntVariable(const int variable_id, int &value) const;
  bool getTextureVariable(const int variable_id, TEXTUREID &value) const;
  bool getSamplerVariable(const int variable_id, d3d::SamplerHandle &value) const;
  /// @}

  /// @name Dispatch
  /// @{

  /// Returns the thread group sizes (X, Y, Z) declared in the shader.
  eastl::array<uint16_t, 3> getThreadGroupSizes() const;

  /// Dispatches the specified number of thread groups.
  /// @param tgx Number of thread groups in X.
  /// @param tgy Number of thread groups in Y.
  /// @param tgz Number of thread groups in Z.
  /// @param gpu_pipeline GPU pipeline to dispatch on.
  /// @param set_states If true, binds shader states before dispatch.
  /// @return true on success.
  bool dispatch(int tgx, int tgy, int tgz, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const;

  /// Dispatches by total thread count, automatically dividing by thread group sizes (rounding up).
  /// @param threads_x Total threads in X.
  /// @param threads_y Total threads in Y.
  /// @param threads_z Total threads in Z.
  /// @param gpu_pipeline GPU pipeline to dispatch on.
  /// @param set_states If true, binds shader states before dispatch.
  /// @return true on success.
  bool dispatchThreads(int threads_x, int threads_y, int threads_z, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS,
    bool set_states = true) const;

  /// Returns the wave/warp size for this shader.
  uint32_t getWaveSize() const;

  /// Dispatches a single thread group (1, 1, 1).
  bool dispatch(GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const
  {
    return dispatch(1, 1, 1, gpu_pipeline, set_states);
  }

  /// Dispatches using indirect arguments read from a GPU buffer.
  /// @param args Buffer containing dispatch arguments (3 uint32_t values: groupCountX, groupCountY, groupCountZ).
  ///   Must be created with SBCF_BIND_UNORDERED and SBCF_MISC_DRAWINDIRECT flags
  ///   (e.g. via d3d::buffers::create_ua_indirect with Indirect::Dispatch).
  /// @param ofs Byte offset into @p args where the arguments start.
  ///   Must be a multiple of DISPATCH_INDIRECT_BUFFER_SIZE (12 bytes) when accessing
  ///   sequential dispatch entries in the buffer.
  /// @param gpu_pipeline GPU pipeline to dispatch on.
  /// @param set_states If true, binds shader states before dispatch.
  /// @return true on success.
  bool dispatch_indirect(Sbuffer *args, int ofs, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS, bool set_states = true) const;

  /// @}

  /// Binds shader states without dispatching.
  bool setStates() const;

  /// Returns the underlying compute program handle.
  PROGRAM getComputeProgram() const;

protected:
  ScriptedShaderMaterial *mat;
  ScriptedShaderElement *elem;
};


/// Creates a ComputeShaderElement from a DSHL shader class name.
/// @param shader_name Name of the compute shader class defined in DSHL.
/// @param optional If true, returns nullptr instead of asserting when the shader is not found.
/// @return Pointer to a new ComputeShaderElement, or nullptr if @p optional is true and the shader was not found.
ComputeShaderElement *new_compute_shader(const char *shader_name, bool optional = false);

/// Lightweight RAII wrapper around ComputeShaderElement for common dispatch patterns.
///
/// Manages the lifetime of the underlying ComputeShaderElement and provides
/// a simplified dispatch interface that always uses GpuPipeline::GRAPHICS.
class ComputeShader
{
  eastl::unique_ptr<ComputeShaderElement> elem;

public:
  ComputeShader() = default;

  /// Constructs a compute shader from a DSHL shader class name.
  /// @param shader_name Name of the compute shader class. If nullptr, creates an empty (invalid) shader.
  /// @param optional If true, silently handles missing shaders instead of asserting.
  ComputeShader(const char *shader_name, bool optional = false) :
    elem(shader_name ? new_compute_shader(shader_name, optional) : nullptr)
  {}

  /// Dispatches by total thread count, auto-dividing by thread group sizes.
  bool dispatchThreads(int threads_x, int threads_y, int threads_z) const
  {
    return elem->dispatchThreads(threads_x, threads_y, threads_z);
  }

  /// Dispatches the specified number of thread groups.
  bool dispatchGroups(int groups_x, int groups_y, int groups_z) const { return elem->dispatch(groups_x, groups_y, groups_z); }

  /// Dispatches using indirect arguments from a GPU buffer.
  bool dispatchIndirect(Sbuffer *args, int ofs) const { return elem->dispatch_indirect(args, ofs); }

  /// Returns the underlying compute program handle.
  PROGRAM getComputeProgram() const { return elem->getComputeProgram(); }

  /// Returns true if the shader was loaded successfully.
  explicit operator bool() const { return elem != nullptr; }
};
