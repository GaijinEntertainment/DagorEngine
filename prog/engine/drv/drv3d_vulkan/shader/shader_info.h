// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <drv/3d/dag_consts.h>

#include "vulkan_device.h"

namespace drv3d_vulkan
{

class DeviceContext;

struct ShaderIdentifier
{
  spirv::HashValue headerHash;
  spirv::HashValue shaderHash;
  uint32_t shaderSize;
};

inline bool operator==(const ShaderIdentifier &l, const ShaderIdentifier &r)
{
  return (l.shaderSize == r.shaderSize) && (l.shaderHash == r.shaderHash) && (l.headerHash == r.headerHash);
}

inline bool operator!=(const ShaderIdentifier &l, const ShaderIdentifier &r) { return !(l == r); }

struct UniqueShaderPart
{
  uint32_t id = 0;
  uint32_t refCount = 1;

  UniqueShaderPart() = default;
  bool release() { return --refCount == 0; }
  void onDuplicateAddition() { ++refCount; }
  static constexpr bool alwaysUnique() { return false; }
  static uint32_t makeID(LinearStorageIndex index) { return index; }
  static bool checkID(uint32_t) { return true; }
  static LinearStorageIndex getIndexFromID(uint32_t id) { return id; }
};

// Shaders are formed from two unique parts.
// The header, it specifies the resource mapping.
// And the shader module it self.
// Due to our shader system and the behavior of
// the vulkan shader compiler, it is possible that
// different shaders only differ by their resource
// mapping but share the same shader code.

struct UniqueShaderHeader : ShaderModuleHeader, UniqueShaderPart
{
  typedef ShaderModuleHeader CreationInfo;

  UniqueShaderHeader(const CreationInfo &info) { static_cast<ShaderModuleHeader &>(*this) = info; }

  bool isSame(const CreationInfo &info) const { return (stage == info.stage) && (hash == info.hash) && (header == info.header); }

  void removeFromContext(DeviceContext &, uint32_t) {}
  void addToContext(DeviceContext &, uint32_t db_id, const CreationInfo &) { id = db_id; }
  static constexpr bool isRemovalPending() { return false; }
};

struct UniqueShaderModule : UniqueShaderPart
{
  spirv::HashValue hash{};
  uint32_t size = 0;
  bool inRemoval = false;

  typedef ShaderModuleBlob CreationInfo;

  UniqueShaderModule(const CreationInfo &info) : hash(info.hash), size(info.getBlobSize()) {}

  bool isSame(const CreationInfo &info) const { return size == info.getBlobSize() && hash == info.hash; }

  void addToContext(DeviceContext &ctx, uint32_t id, const CreationInfo &info);
  void removeFromContext(DeviceContext &ctx, uint32_t id);
  bool isRemovalPending() { return inRemoval; }
};

struct ShaderInfo
{
  UniqueShaderHeader *header = nullptr;
  UniqueShaderModule *module = nullptr;
  eastl::unique_ptr<ShaderInfo> geometryShader;
  eastl::unique_ptr<ShaderInfo> controlShader;
  eastl::unique_ptr<ShaderInfo> evaluationShader;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  ShaderDebugInfo debugInfo;

  void setDebugName(const char *val)
  {
    if ((debugInfo.debugName != val) && (debugInfo.debugName != "<unknown>"))
    {
      // shader can be duplicated and if its name is not same as before,
      // this is can be a bad sign, so log it at least
      debug("vulkan: duplicated shader with different debug name new %s old %s", val, debugInfo.debugName.c_str());
    }
    debugInfo.debugName = val;
  }

#endif
  uint32_t refCount = 1;

  struct CreationInfo
  {
    UniqueShaderHeader *header_a = nullptr;
    UniqueShaderModule *module_a = nullptr;
    UniqueShaderHeader *header_b = nullptr;
    UniqueShaderModule *module_b = nullptr;
    UniqueShaderHeader *header_c = nullptr;
    UniqueShaderModule *module_c = nullptr;
    UniqueShaderHeader *header_d = nullptr;
    UniqueShaderModule *module_d = nullptr;

    uint32_t getHash32() const
    {
      return mem_hash_fnv1<32>((const char *)&header_a, sizeof(header_a)) ^
             mem_hash_fnv1<32>((const char *)&module_a, sizeof(module_a));
    }
  };

  ShaderInfo(UniqueShaderHeader *s_header, UniqueShaderModule *s_module) : header(s_header), module(s_module) {}

  ShaderInfo(const CreationInfo &info) : header(info.header_a), module(info.module_a)
  {
    if (info.header_b && info.module_b)
      geometryShader = eastl::make_unique<ShaderInfo>(info.header_b, info.module_b);

    if (info.header_c && info.module_c)
      controlShader = eastl::make_unique<ShaderInfo>(info.header_c, info.module_c);

    if (info.header_d && info.module_d)
      evaluationShader = eastl::make_unique<ShaderInfo>(info.header_d, info.module_d);
  }

  bool isSame(const CreationInfo &info)
  {
    if ((module != info.module_a) || (header != info.header_a))
      return false;

    if (geometryShader)
    {
      if ((geometryShader->module != info.module_b) || (geometryShader->header != info.header_b))
        return false;
    }
    else if (info.module_b)
      return false;

    if (controlShader)
    {
      if ((controlShader->module != info.module_c) || (controlShader->header != info.header_c))
        return false;
    }
    else if (info.module_c)
      return false;

    if (evaluationShader)
    {
      if ((evaluationShader->module != info.module_d) || (evaluationShader->header != info.header_d))
        return false;
    }
    else if (info.module_d)
      return false;

    return true;
  }

  bool isSame(VkShaderStageFlags shader_stage, const spirv::HashValue &header_hash, const spirv::HashValue &shader_hash,
    intptr_t shader_size) const
  {
    return (header->stage == shader_stage) && (module->size == shader_size) && (header->hash == header_hash) &&
           (module->hash == shader_hash);
  }

  void onDuplicateAddition()
  {
    ++refCount;
    // deduplucate case, but headers and modules had
    // their ref counter increased already so release
    // them
    header->release();
    module->release();
    if (geometryShader)
    {
      geometryShader->header->release();
      geometryShader->module->release();
    }
    if (controlShader)
    {
      controlShader->header->release();
      controlShader->module->release();
    }
    if (evaluationShader)
    {
      evaluationShader->header->release();
      evaluationShader->module->release();
    }
  }

  void addToContext(DeviceContext &, ShaderID, const CreationInfo &) {}
  void removeFromContext(DeviceContext &, ShaderID) {}
  bool release() { return --refCount == 0; }
  static constexpr bool isRemovalPending() { return false; }
  static constexpr bool alwaysUnique() { return false; }
  static ShaderID makeID(LinearStorageIndex index) { return ShaderID(index); }
  static bool checkID(ShaderID) { return true; }
  static LinearStorageIndex getIndexFromID(ShaderID id) { return id.get(); }

  const spirv::ShaderHeader &getHeader() const { return header->header; }

  VkShaderStageFlags getStage() const { return header->stage; }

  ShaderIdentifier getIdentifier() const
  {
    ShaderIdentifier result;
    result.headerHash = header->hash;
    result.shaderHash = module->hash;
    result.shaderSize = module->size;
    return result;
  }

  ShaderModuleUse getUseInfo() const
  {
    ShaderModuleUse result;
    result.blobId = module->id;
    result.header = *header;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    result.dbg = debugInfo;
#endif

    return result;
  }
};

} // namespace drv3d_vulkan