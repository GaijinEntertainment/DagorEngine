// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "dynamic_array.h"

#include <drv/3d/dag_shaderLibrary.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <EASTL/string.h>
#include <supp/dag_comPtr.h>


#if !_TARGET_XBOXONE
namespace drv3d_dx12
{
class ShaderLibrary
{
  eastl::string libName;
  dxil::LibraryResourceInformation libraryResourceInfo;
  DynamicArray<dxil::LibraryShaderProperties> shaderProperties;
  DynamicArray<DynamicArray<wchar_t>> nameTable;
  ComPtr<ID3D12StateObject> object;
  bool allowExpandableUse;

  ShaderLibrary() = delete;
  ShaderLibrary(const ShaderLibrary &) = delete;
  ShaderLibrary(ShaderLibrary &&) = delete;
  ShaderLibrary &operator=(const ShaderLibrary &) = delete;
  ShaderLibrary &operator=(ShaderLibrary &&) = delete;

public:
  ~ShaderLibrary() = default;
  ShaderLibrary(eastl::string &&lib_name, const dxil::LibraryResourceInformation &lib_res_info,
    dag::ConstSpan<dxil::LibraryShaderProperties> shader_properties, DynamicArray<DynamicArray<wchar_t>> &&name_table,
    ComPtr<ID3D12StateObject> &&obj, bool allow_expandable_use) :
    libName{eastl::move(lib_name)},
    libraryResourceInfo{lib_res_info},
    nameTable{eastl::move(name_table)},
    object{eastl::move(obj)},
    allowExpandableUse{allow_expandable_use}
  {
    shaderProperties.resize(shader_properties.size());
    eastl::copy(shader_properties.begin(), shader_properties.end(), shaderProperties.data());
  }

  ID3D12StateObject *get() const { return object.Get(); }
  const dxil::LibraryShaderProperties &getShaderPropertiesOf(uint32_t index) const { return shaderProperties[index]; }
  const dxil::LibraryResourceInformation &getGlobalResourceInfo() const { return libraryResourceInfo; }
  size_t shaderCount() const { return shaderProperties.size(); }
  bool canBeUsedByExpandablePipelines() const { return allowExpandableUse; }
  const eastl::string &name() const { return libName; }
  eastl::wstring_view nameOf(uint32_t index) const;

  static ShaderLibrary *build(ID3D12Device5 *device, const ::ShaderLibraryCreateInfo &ci);
};
} // namespace drv3d_dx12
#endif