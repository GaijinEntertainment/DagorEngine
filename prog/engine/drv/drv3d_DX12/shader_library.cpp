// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shader_library.h"
#include "d3d12_error_handling.h"

#include <perfMon/dag_autoFuncProf.h>


namespace
{
template <typename T>
class ConcatWrapper;

template <>
class ConcatWrapper<DynamicArray<wchar_t>>
{
  DynamicArray<wchar_t> &target;
  size_t pos = 0;

public:
  // assumes t is null terminated when not empty
  ConcatWrapper(DynamicArray<wchar_t> &t, size_t concat_size) : target{t}, pos{t.empty() ? 0 : t.size() - 1}
  {
    target.resize(pos + concat_size + 1);
  }
  ~ConcatWrapper() { append(L'\0'); }

  void append(eastl::wstring_view w_str)
  {
    eastl::copy(w_str.begin(), w_str.end(), target.data() + pos);
    pos += w_str.length();
  }
  void append(wchar_t wc) { target[pos++] = wc; }
  void append(const char *str, size_t len)
  {
    eastl::copy(str, str + len, target.data() + pos);
    pos += len;
  }
};

template <>
class ConcatWrapper<eastl::wstring>
{
  eastl::wstring &target;

public:
  ConcatWrapper(eastl::wstring &t, size_t concat_size) : target{t} { target.reserve(target.size() + concat_size); }

  void append(eastl::wstring_view w_str) { target.append(w_str.begin(), w_str.end()); }
  void append(wchar_t wc) { target.push_back(wc); }
  void append(const char *str, size_t len) { eastl::copy(str, str + len, eastl::back_inserter(target)); }
};

template <typename T>
inline void append_name_for_shader_lib_at(eastl::wstring_view lib_prefix, uint32_t index, T &target)
{
  char buf[5];
  auto count = sprintf_s(buf, "%04X", index);

  ConcatWrapper<T> doConcat{target, lib_prefix.length() + count + 1};
  doConcat.append(lib_prefix);
  doConcat.append(L'_');
  doConcat.append(buf, count);
}
template <unsigned C>
struct AutoLifetimeTimer
{
  eastl::string timingFormatString;
  AutoFuncProfT<C> timingData;
  template <typename T>
  AutoLifetimeTimer(T maker) : timingFormatString{maker()}, timingData{timingFormatString.c_str()}
  {}

  void abbortTiming() { timingData.fmt = nullptr; }
};

class ShaderLibraryBuilder : AutoLifetimeTimer<AFP_MSEC>
{
  const ::ShaderLibraryCreateInfo &ci;

  template <typename... Ts>
  drv3d_dx12::ShaderLibrary *onError(Ts &&...ts)
  {
    abbortTiming();
    logwarn(eastl::forward<Ts>(ts)...);
    return nullptr;
  }

public:
  ShaderLibraryBuilder(const ::ShaderLibraryCreateInfo &create_info) :
    AutoLifetimeTimer<AFP_MSEC>{[&create_info]() {
      eastl::string s;
      s = "DX12: ShaderLibraryBuilder: Building library <";
      s += create_info.debugName ? create_info.debugName : "UNKNOWN";
      s += "> took %ums";
      return s;
    }},
    ci{create_info}
  {}

  drv3d_dx12::ShaderLibrary *build(ID3D12Device5 *device)
  {
    eastl::string libName;
    if (ci.debugName)
    {
      libName = ci.debugName;
    }

    logdbg("DX12: Building new shader library <%s>...", libName);
    if (ci.driverBinary.empty())
    {
      return onError("DX12: Create info for shader library <%s> had no binary", libName);
    }

    auto libContainer = bindump::map<dxil::ShaderLibraryContainer>(ci.driverBinary.data());
    if (!libContainer)
    {
      return onError("DX12: Tried to load non library byte code as shader library for <%s>", libName);
    }

    // when exported shader count does not match name table count, something is broken
    if (libContainer->shaderProperties.size() != ci.nameTable.size())
    {
      return onError("DX12: Name table size (%u) does not match count of exported shaders of shader binary (%u) for <%s>",
        ci.nameTable.size(), libContainer->shaderProperties.size(), libName);
    }

    if (ShaderHashValue::calculate(libContainer->dxilBinary.data(), libContainer->dxilBinary.size()) != libContainer->binaryHash)
    {
      return onError("DX12: Shader library calculated hash does not match stored hash for <%s>", libName);
    }

    char strBuf[sizeof(ShaderHashValue) * 2 + 1];
    libContainer->binaryHash.convertToString(strBuf, sizeof(strBuf));
    eastl::wstring libPrefix;
    // -1, as we don't need to push back the terminating 0 into the string
    eastl::copy(eastl::begin(strBuf), eastl::end(strBuf) - 1, eastl::back_inserter(libPrefix));

    DynamicArray<D3D12_EXPORT_DESC> exportTable;
    DynamicArray<DynamicArray<wchar_t>> nameTable;
    exportTable.resize(libContainer->shaderProperties.size());
    nameTable.resize(libContainer->shaderProperties.size() * 2);
    for (uint32_t i = 0; i < libContainer->shaderProperties.size(); ++i)
    {
      eastl::string_view srcName = ci.nameTable[i];
      auto &inHLSLName = nameTable[i * 2 + 0];
      auto &publicName = nameTable[i * 2 + 1];
      inHLSLName.resize(srcName.length() + 1);
      inHLSLName[srcName.length()] = L'\0';
      eastl::copy(srcName.begin(), srcName.end(), inHLSLName.data());
      append_name_for_shader_lib_at(libPrefix, i, publicName);
      auto &desc = exportTable[i];
      desc.Name = publicName.data();
      desc.ExportToRename = inHLSLName.data();
      desc.Flags = D3D12_EXPORT_FLAG_NONE;
    }

    D3D12_DXIL_LIBRARY_DESC libDesc{};
    libDesc.DXILLibrary.pShaderBytecode = libContainer->dxilBinary.data();
    libDesc.DXILLibrary.BytecodeLength = libContainer->dxilBinary.size();
    libDesc.NumExports = exportTable.size();
    libDesc.pExports = exportTable.data();

    D3D12_STATE_OBJECT_CONFIG config{};
    // we allow state objects to be incomplete so that they can be used to glue stuff together
    // we have to see if this is possible with other APIs
    config.Flags = D3D12_STATE_OBJECT_FLAGS(
      D3D12_STATE_OBJECT_FLAG_ALLOW_LOCAL_DEPENDENCIES_ON_EXTERNAL_DEFINITIONS |
      D3D12_STATE_OBJECT_FLAG_ALLOW_EXTERNAL_DEPENDENCIES_ON_LOCAL_DEFINITIONS |
      (ci.mayBeUsedByExpandablePipeline ? D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS : D3D12_STATE_OBJECT_FLAG_NONE));

    D3D12_STATE_SUBOBJECT objectRefs[2]{};
    objectRefs[0].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    objectRefs[0].pDesc = &libDesc;
    objectRefs[1].Type = D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG;
    objectRefs[1].pDesc = &config;

    D3D12_STATE_OBJECT_DESC desc{};
    desc.Type = D3D12_STATE_OBJECT_TYPE_COLLECTION;
    desc.NumSubobjects = countof(objectRefs);
    desc.pSubobjects = objectRefs;

    ComPtr<ID3D12StateObject> object;
    if (DX12_CHECK_FAIL(device->CreateStateObject(&desc, COM_ARGS(&object))))
    {
      return onError("DX12: Unable to create shader library <%s>", libName);
    }

    logdbg("DX12: ...completed, creating shader library object for <%s>...", libName);

    return new drv3d_dx12::ShaderLibrary(eastl::move(libName), eastl::move(libPrefix), libContainer->resourceUsageInfo,
      libContainer->shaderProperties, eastl::move(object), ci.mayBeUsedByExpandablePipeline);
  }
};
} // namespace

void drv3d_dx12::ShaderLibrary::appendNameOfTo(uint32_t index, eastl::wstring &target) const
{
  if (index >= shaderProperties.size())
  {
    return;
  }
  append_name_for_shader_lib_at(libPrefix, index, target);
}
DynamicArray<wchar_t> drv3d_dx12::ShaderLibrary::makeNameOfAsDynArray(uint32_t index) const
{
  DynamicArray<wchar_t> result;
  if (index < shaderProperties.size())
  {
    append_name_for_shader_lib_at(libPrefix, index, result);
  }
  return result;
}

drv3d_dx12::ShaderLibrary *drv3d_dx12::ShaderLibrary::build(ID3D12Device5 *device, const ::ShaderLibraryCreateInfo &ci)
{
  ShaderLibraryBuilder shaderLibraryBuilder{ci};
  return shaderLibraryBuilder.build(device);
}
