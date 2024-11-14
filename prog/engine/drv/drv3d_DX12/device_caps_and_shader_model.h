// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "d3d_cap_set_xmacro.h"
#include "driver.h"
#include <drv/3d/dag_consts.h>


struct DeviceCapsAndShaderModel
{
  // deliberately using base to allow to load all cap values
  DeviceDriverCapabilitiesBase caps;
  d3d::shadermodel::Version shaderModel;

  bool isCompatibleTo(d3d::shadermodel::Version shader_model) const { return shader_model >= shaderModel; }
  bool isCompatibleTo(const DeviceDriverCapabilities &other) const
  {
    // This is a very simple approach, when a feature of other is requested but not indicated by caps, we are not compatible.
#define DX12_D3D_CAP(name)      \
  if (caps.name && !other.name) \
    return false;
    DX12_D3D_CAP_SET
#undef DX12_D3D_CAP
    return true;
  }
  bool isPipelineCompatibleTo(const DeviceDriverCapabilities &other) const
  {
    // This is a very simple approach, when a feature of other is requested but not indicated by caps, we are not compatible.
#define DX12_D3D_CAP(name)      \
  if (caps.name && !other.name) \
    return false;
    DX12_D3D_CAP_SET_RELEVANT_FOR_PIPELINES
#undef DX12_D3D_CAP
    return true;
  }
  bool isCompatibleTo(const Driver3dDesc &desc) const { return isCompatibleTo(desc.shaderModel) && isCompatibleTo(desc.caps); }
  bool isPipelineCompatibleTo(const Driver3dDesc &desc) const
  {
    return isCompatibleTo(desc.shaderModel) && isPipelineCompatibleTo(desc.caps);
  }
  static DeviceCapsAndShaderModel fromDriverDesc(const Driver3dDesc &desc)
  {
    DeviceCapsAndShaderModel result;
    result.shaderModel = desc.shaderModel;
    // need to do a copy this way to properly copy constants into variables.
#define DX12_D3D_CAP(name) result.caps.name = desc.caps.name;
    DX12_D3D_CAP_SET
#undef DX12_D3D_CAP
    return result;
  }
};

inline d3d::shadermodel::Version shader_model_from_dx(D3D_SHADER_MODEL model)
{
  unsigned int ma = (model >> 4) & 0xF;
  unsigned int mi = (model >> 0) & 0xF;
  return {ma, mi};
}

inline D3D_SHADER_MODEL shader_model_to_dx(d3d::shadermodel::Version model)
{
  return static_cast<D3D_SHADER_MODEL>(model.major << 4 | model.minor);
}
