// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "dynamic_array.h"

#include <drv/3d/dag_shaderLibrary.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>
#include <supp/dag_comPtr.h>


#if !_TARGET_XBOXONE
namespace drv3d_dx12
{
class ShaderLibrary
{
  eastl::string libName;
  eastl::wstring libPrefix;
  dxil::LibraryResourceInformation libraryResourceInfo;
  DynamicArray<dxil::LibraryShaderProperties> shaderProperties;
  ComPtr<ID3D12StateObject> object;
  bool allowExpandableUse;

  ShaderLibrary() = delete;
  ShaderLibrary(const ShaderLibrary &) = delete;
  ShaderLibrary(ShaderLibrary &&) = delete;
  ShaderLibrary &operator=(const ShaderLibrary &) = delete;
  ShaderLibrary &operator=(ShaderLibrary &&) = delete;

public:
  ~ShaderLibrary() = default;
  ShaderLibrary(eastl::string &&lib_name, eastl::wstring &&lib_prefix, const dxil::LibraryResourceInformation &lib_res_info,
    dag::ConstSpan<dxil::LibraryShaderProperties> shader_properties, ComPtr<ID3D12StateObject> &&obj, bool allow_expandable_use) :
    libName{eastl::move(lib_name)},
    libPrefix{eastl::move(lib_prefix)},
    libraryResourceInfo{lib_res_info},
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
  const eastl::wstring &prefix() const { return libPrefix; }
  void appendNameOfTo(uint32_t index, eastl::wstring &target) const;
  DynamicArray<wchar_t> makeNameOfAsDynArray(uint32_t index) const;

  static ShaderLibrary *build(ID3D12Device5 *device, const ::ShaderLibraryCreateInfo &ci);
};
} // namespace drv3d_dx12
#endif