//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "compiled_shader_header.h"

namespace dxil
{

template <typename T, typename F>
inline void decode_root_constant_buffer(const ShaderResourceUsageTable &header, T &clb, F &&set_visibility_function)
{
  if (header.rootConstantDwords > 0)
  {
    set_visibility_function();
    clb.rootConstantBuffer(ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET + header.rootConstantDwords, ROOT_CONSTANT_BUFFER_REGISTER_INDEX,
      header.rootConstantDwords);
  }
}

template <typename T>
inline void decode_special_constants(uint8_t special_constants_mask, T &clb)
{
  if (special_constants_mask & SC_DRAW_ID)
  {
    clb.specialConstants(DRAW_ID_REGISTER_SPACE, SPECIAL_CONSTANTS_REGISTER_INDEX);
  }
}

template <typename T, typename F>
inline void decode_constant_buffers(const ShaderResourceUsageTable &header, uint32_t combined_mask, T &clb,
  F &&set_visibility_function)
{
  uint32_t mask = header.bRegisterUseMask;
  if (!mask)
    return;

  set_visibility_function();

  clb.beginConstantBuffers();
  uint32_t linearIndex = 0;
  for (uint32_t i = 0; i < MAX_B_REGISTERS && mask; mask >>= 1, ++i, combined_mask >>= 1)
  {
    if (mask & 1)
    {
      clb.constantBuffer(REGULAR_RESOURCES_SPACE_INDEX, i, linearIndex);
    }
    linearIndex += combined_mask & 1;
  }
  clb.endConstantBuffers();
}

template <typename T, typename F>
inline void decode_samplers(const ShaderResourceUsageTable &header, uint32_t combined_mask, T &clb, F &&set_visibility_function)
{
  uint32_t mask = header.sRegisterUseMask;
  if (!mask)
    return;

  set_visibility_function();

  clb.beginSamplers();
  uint32_t linearIndex = 0;
  for (uint32_t i = 0; i < MAX_S_REGISTERS && mask; mask >>= 1, ++i, combined_mask >>= 1)
  {
    if (mask & 1)
    {
      clb.sampler(REGULAR_RESOURCES_SPACE_INDEX, i, linearIndex);
    }
    linearIndex += combined_mask & 1;
  }
  clb.endSamplers();
}

template <typename T, typename F>
inline void decode_bindless_samplers(const ShaderResourceUsageTable &header, T &clb, F &&set_visibility_function)
{
  uint32_t mask = (header.bindlessUsageMask & BINDLESS_SAMPLERS_SPACE_BITS_MASK) >> BINDLESS_SAMPLERS_SPACE_BITS_SHIFT;
  if (!mask)
    return;

  set_visibility_function();

  clb.beginBindlessSamplers();
  for (uint32_t i = 0; i < BINDLESS_SAMPLERS_SPACE_COUNT && mask; mask >>= 1, ++i)
  {
    if (mask & 1)
    {
      clb.bindlessSamplers(BINDLESS_SAMPLERS_SPACE_OFFSET + i, BINDLESS_REGISTER_INDEX);
    }
  }
  clb.endBindlessSamplers();
}

template <typename T, typename F>
inline void decode_shader_resource_views(const ShaderResourceUsageTable &header, uint32_t combined_mask, T &clb,
  F &&set_visibility_function)
{
  uint32_t mask = header.tRegisterUseMask;
  if (!mask)
    return;

  set_visibility_function();

  clb.beginShaderResourceViews();
  uint32_t lastIndex = 0, descriptorCount = 0;
  uint32_t linearIndex = 0, lastLinearIndex = 0;
  for (uint32_t i = 0; i <= MAX_T_REGISTERS && mask; mask >>= 1, ++i, combined_mask >>= 1)
  {
    if (mask & 1)
    {
      if (!descriptorCount)
      {
        lastIndex = i;
        lastLinearIndex = linearIndex;
      }
      ++descriptorCount;
      // look ahead to the next bit we are going to inspect
      if (0 == (mask & 0b10))
      {
        clb.shaderResourceView(REGULAR_RESOURCES_SPACE_INDEX, lastIndex, descriptorCount, lastLinearIndex);
        descriptorCount = 0;
      }
    }
    linearIndex += combined_mask & 1;
  }
  clb.endShaderResourceViews();
}

template <typename T, typename F>
inline void decode_bindless_shader_resource_views(const ShaderResourceUsageTable &header, T &clb, F set_visibility_function)
{
  uint32_t mask = (header.bindlessUsageMask & BINDLESS_RESOURCES_SPACE_BITS_MASK) >> BINDLESS_RESOURCES_SPACE_BITS_SHIFT;
  if (!mask)
    return;

  set_visibility_function();

  clb.beginBindlessShaderResourceViews();
  for (uint32_t i = 0; i < BINDLESS_RESOURCES_SPACE_COUNT && mask; mask >>= 1, ++i)
  {
    if (mask & 1)
    {
      clb.bindlessShaderResourceViews(BINDLESS_RESOURCES_SPACE_OFFSET + i, BINDLESS_REGISTER_INDEX);
    }
  }
  clb.endBindlessShaderResourceViews();
}

template <typename T, typename F>
inline void decode_unordered_access_views(const ShaderResourceUsageTable &header, uint32_t combined_mask, T &clb,
  F &&set_visibility_function)
{
  uint32_t mask = header.uRegisterUseMask;
  if (!mask)
    return;

  set_visibility_function();

  clb.beginUnorderedAccessViews();

  uint32_t lastIndex = 0, descriptorCount = 0;
  uint32_t linearIndex = 0, lastLinearIndex = 0;
  for (uint32_t i = 0; i <= MAX_U_REGISTERS && mask; mask >>= 1, ++i, combined_mask >>= 1)
  {
    if (mask & 1)
    {
      if (!descriptorCount)
      {
        lastIndex = i;
        lastLinearIndex = linearIndex;
      }
      ++descriptorCount;
      // look ahead to the next bit we are going to inspect
      if (0 == (mask & 0b10))
      {
        clb.unorderedAccessView(REGULAR_RESOURCES_SPACE_INDEX, lastIndex, descriptorCount, lastLinearIndex);
        descriptorCount = 0;
      }
    }
    linearIndex += combined_mask & 1;
  }

  clb.endUnorderedAccessViews();
}

// Helper to uniformly generate a root signature from a shader head of a compute shader
template <typename T>
inline void decode_compute_root_signature(bool has_acceleration_structure, const ShaderResourceUsageTable &header, T &clb)
{
  clb.begin();

  clb.beginFlags();
  if (has_acceleration_structure)
    clb.hasAccelerationStructure();
  clb.endFlags();

  decode_root_constant_buffer(header, clb, []() {});
  decode_constant_buffers(header, header.bRegisterUseMask, clb, []() {});
  decode_samplers(header, header.sRegisterUseMask, clb, []() {});
  decode_bindless_samplers(header, clb, []() {});
  decode_shader_resource_views(header, header.tRegisterUseMask, clb, []() {});
  decode_bindless_shader_resource_views(header, clb, []() {});
  decode_unordered_access_views(header, header.uRegisterUseMask, clb, []() {});

  clb.end();
}

// Helper to uniformly generate a root signature from a set of shaders for the graphics pipeline.
template <typename T>
inline void decode_graphics_root_signature(bool vs_has_inputs, bool has_acceleration_structure, const ShaderResourceUsageTable &vs,
  const ShaderResourceUsageTable &ps, const ShaderResourceUsageTable &hs, const ShaderResourceUsageTable &ds,
  const ShaderResourceUsageTable &gs, T &clb)
{
  clb.begin();

  clb.beginFlags();
  if (vs_has_inputs)
    clb.hasVertexInputs();
  if (has_acceleration_structure)
    clb.hasAccelerationStructure();
  if (!any_registers_used(vs))
    clb.noVertexShaderResources();
  if (!any_registers_used(ps))
    clb.noPixelShaderResources();
  if (!any_registers_used(hs))
    clb.noHullShaderResources();
  if (!any_registers_used(ds))
    clb.noDomainShaderResources();
  if (!any_registers_used(gs))
    clb.noGeometryShaderResources();
  clb.endFlags();

  // Calling order is important, because in the end it will also be the order of root parameters.
  // Earlier parameters are expected to be accessed faster by the device, especially if the device
  // needs to spill parts of it from registers into memory. This is why we construct root signature
  // this way, i.e. going through constant buffers for all stages first, then moving on to
  // decode_samplers for all stages, then to SRVs, etc.

  uint8_t specialConstantsMask =
    ps.specialConstantsMask | vs.specialConstantsMask | hs.specialConstantsMask | ds.specialConstantsMask | gs.specialConstantsMask;
  decode_special_constants(specialConstantsMask, clb);

  decode_root_constant_buffer(ps, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_root_constant_buffer(vs, clb, [&clb]() { clb.setVisibilityVertexShader(); });
  decode_root_constant_buffer(hs, clb, [&clb]() { clb.setVisibilityHullShader(); });
  decode_root_constant_buffer(ds, clb, [&clb]() { clb.setVisibilityDomainShader(); });
  decode_root_constant_buffer(gs, clb, [&clb]() { clb.setVisibilityGeometryShader(); });

  uint32_t comboMask = vs.bRegisterUseMask | hs.bRegisterUseMask | ds.bRegisterUseMask | gs.bRegisterUseMask;
  decode_constant_buffers(ps, ps.bRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_constant_buffers(vs, comboMask, clb, [&clb]() { clb.setVisibilityVertexShader(); });
  decode_constant_buffers(hs, comboMask, clb, [&clb]() { clb.setVisibilityHullShader(); });
  decode_constant_buffers(ds, comboMask, clb, [&clb]() { clb.setVisibilityDomainShader(); });
  decode_constant_buffers(gs, comboMask, clb, [&clb]() { clb.setVisibilityGeometryShader(); });

  comboMask = vs.sRegisterUseMask | hs.sRegisterUseMask | ds.sRegisterUseMask | gs.sRegisterUseMask;
  decode_samplers(ps, ps.sRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_samplers(vs, comboMask, clb, [&clb]() { clb.setVisibilityVertexShader(); });
  decode_samplers(hs, comboMask, clb, [&clb]() { clb.setVisibilityHullShader(); });
  decode_samplers(ds, comboMask, clb, [&clb]() { clb.setVisibilityDomainShader(); });
  decode_samplers(gs, comboMask, clb, [&clb]() { clb.setVisibilityGeometryShader(); });

  decode_bindless_samplers(ps, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_bindless_samplers(vs, clb, [&clb]() { clb.setVisibilityVertexShader(); });
  decode_bindless_samplers(hs, clb, [&clb]() { clb.setVisibilityHullShader(); });
  decode_bindless_samplers(ds, clb, [&clb]() { clb.setVisibilityDomainShader(); });
  decode_bindless_samplers(gs, clb, [&clb]() { clb.setVisibilityGeometryShader(); });

  comboMask = vs.tRegisterUseMask | hs.tRegisterUseMask | ds.tRegisterUseMask | gs.tRegisterUseMask;
  decode_shader_resource_views(ps, ps.tRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_shader_resource_views(vs, comboMask, clb, [&clb]() { clb.setVisibilityVertexShader(); });
  decode_shader_resource_views(hs, comboMask, clb, [&clb]() { clb.setVisibilityHullShader(); });
  decode_shader_resource_views(ds, comboMask, clb, [&clb]() { clb.setVisibilityDomainShader(); });
  decode_shader_resource_views(gs, comboMask, clb, [&clb]() { clb.setVisibilityGeometryShader(); });

  decode_bindless_shader_resource_views(ps, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_bindless_shader_resource_views(vs, clb, [&clb]() { clb.setVisibilityVertexShader(); });
  decode_bindless_shader_resource_views(hs, clb, [&clb]() { clb.setVisibilityHullShader(); });
  decode_bindless_shader_resource_views(ds, clb, [&clb]() { clb.setVisibilityDomainShader(); });
  decode_bindless_shader_resource_views(gs, clb, [&clb]() { clb.setVisibilityGeometryShader(); });

  comboMask = vs.uRegisterUseMask | hs.uRegisterUseMask | ds.uRegisterUseMask | gs.uRegisterUseMask;
  decode_unordered_access_views(ps, ps.uRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_unordered_access_views(vs, comboMask, clb, [&clb]() { clb.setVisibilityVertexShader(); });
  decode_unordered_access_views(hs, comboMask, clb, [&clb]() { clb.setVisibilityHullShader(); });
  decode_unordered_access_views(ds, comboMask, clb, [&clb]() { clb.setVisibilityDomainShader(); });
  decode_unordered_access_views(gs, comboMask, clb, [&clb]() { clb.setVisibilityGeometryShader(); });

  clb.end();
}

// Helper to uniformly generate a root signature from a set of shaders for the graphics pipeline using mesh shader stage.
// Unlike other decoders pass nullptr if no amplification stage is used by the pipeline, otherwise the root signature
// may be not optimal for mesh shader only pipelines.
template <typename T>
inline void decode_graphics_mesh_root_signature(bool has_acceleration_structure, const ShaderResourceUsageTable &ms,
  const ShaderResourceUsageTable &ps, const ShaderResourceUsageTable *as, T &clb)
{
  clb.begin();

  clb.beginFlags();
  if (as)
    clb.hasAmplificationStage();
  if (has_acceleration_structure)
    clb.hasAccelerationStructure();
  if (!any_registers_used(ms))
    clb.noMeshShaderResources();
  if (!any_registers_used(ps))
    clb.noPixelShaderResources();
  if (!as || !any_registers_used(*as))
    clb.noAmplificationShaderResources();
  clb.endFlags();

  // Calling order is important, because in the end it will also be the order of root parameters.
  // Earlier parameters are expected to be accessed faster by the device, especially if the device
  // needs to spill parts of it from registers into memory. This is why we construct root signature
  // this way, i.e. going through constant buffers for all stages first, then moving on to
  // decode_samplers for all stages, then to SRVs, etc.

  uint8_t specialConstantsMask = ps.specialConstantsMask | ms.specialConstantsMask;
  if (as)
    specialConstantsMask |= as->specialConstantsMask;

  decode_special_constants(specialConstantsMask, clb);

  decode_root_constant_buffer(ps, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_root_constant_buffer(ms, clb, [&clb]() { clb.setVisibilityMeshShader(); });
  if (as)
    decode_root_constant_buffer(*as, clb, [&clb]() { clb.setVisibilityAmplificationShader(); });


  uint32_t comboMask = ms.bRegisterUseMask;
  if (as)
    comboMask |= as->bRegisterUseMask;

  decode_constant_buffers(ps, ps.bRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_constant_buffers(ms, comboMask, clb, [&clb]() { clb.setVisibilityMeshShader(); });
  if (as)
    decode_constant_buffers(*as, comboMask, clb, [&clb]() { clb.setVisibilityAmplificationShader(); });


  comboMask = ms.sRegisterUseMask;
  if (as)
    comboMask |= as->sRegisterUseMask;

  decode_samplers(ps, ps.sRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_samplers(ms, comboMask, clb, [&clb]() { clb.setVisibilityMeshShader(); });
  if (as)
    decode_samplers(*as, comboMask, clb, [&clb]() { clb.setVisibilityAmplificationShader(); });


  decode_bindless_samplers(ps, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_bindless_samplers(ms, clb, [&clb]() { clb.setVisibilityMeshShader(); });
  if (as)
    decode_bindless_samplers(*as, clb, [&clb]() { clb.setVisibilityAmplificationShader(); });


  comboMask = ms.tRegisterUseMask;
  if (as)
    comboMask |= as->tRegisterUseMask;
  decode_shader_resource_views(ps, ps.tRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_shader_resource_views(ms, comboMask, clb, [&clb]() { clb.setVisibilityMeshShader(); });
  if (as)
    decode_shader_resource_views(*as, comboMask, clb, [&clb]() { clb.setVisibilityAmplificationShader(); });


  decode_bindless_shader_resource_views(ps, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_bindless_shader_resource_views(ms, clb, [&clb]() { clb.setVisibilityMeshShader(); });
  if (as)
    decode_bindless_shader_resource_views(*as, clb, [&clb]() { clb.setVisibilityAmplificationShader(); });

  comboMask = ms.uRegisterUseMask;
  if (as)
    comboMask |= as->uRegisterUseMask;
  decode_unordered_access_views(ps, ps.uRegisterUseMask, clb, [&clb]() { clb.setVisibilityPixelShader(); });
  decode_unordered_access_views(ms, comboMask, clb, [&clb]() { clb.setVisibilityMeshShader(); });
  if (as)
    decode_unordered_access_views(*as, comboMask, clb, [&clb]() { clb.setVisibilityAmplificationShader(); });

  clb.end();
}

} // namespace dxil
