// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_consts.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector_multimap.h>
#include <osApiWrappers/dag_critSec.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING

#include "shader.h"
#include "vulkan_device.h"

namespace drv3d_vulkan
{
class DeviceContext;

template <typename ItemType, typename ItemID>
class ShaderProgramDatabaseStorage
{
public:
  ShaderProgramDatabaseStorage() = default;
  ShaderProgramDatabaseStorage(const ShaderProgramDatabaseStorage &) = delete;
  ShaderProgramDatabaseStorage &operator=(const ShaderProgramDatabaseStorage &) = delete;

  void clear(DeviceContext &ctx)
  {
    for (LinearStorageIndex i = 0; i < items.size(); ++i)
    {
      auto &target = items[i];
      if (target && !target->isRemovalPending())
        target->removeFromContext(ctx, ItemType::makeID(i));
    }
  }

  void shutdown()
  {
    items.clear();
    freeIds.clear();
    itemsHashToIndex.clear();
    itemsIndexToHash.clear();
  }

  void remove(DeviceContext &ctx, ItemID id, bool force = false)
  {
    LinearStorageIndex index = ItemType::getIndexFromID(id);
    auto &target = items[index];
    if (target->isRemovalPending() || (!target->release() && !force))
      return;

    target->removeFromContext(ctx, id);
  }

  void reuseId(ItemID id)
  {
    LinearStorageIndex index = ItemType::getIndexFromID(id);
    auto &target = items[index];
    target.reset();

    freeIds.push_back(ItemType::getIndexFromID(id));

    auto hashIter = itemsIndexToHash.find(index);
    G_ASSERT(hashIter != itemsIndexToHash.end());
    auto indexRange = itemsHashToIndex.equal_range(hashIter->second);
    G_ASSERT(indexRange.first != indexRange.second);
    for (auto indexIter = indexRange.first; indexIter != indexRange.second; ++indexIter)
      if (indexIter->second == index)
      {
        itemsHashToIndex.erase(indexIter);
        break;
      }
    itemsIndexToHash.erase(hashIter);
  }

  ItemType *uniqueInsert(DeviceContext &ctx, typename ItemType::CreationInfo info) { return get(add(ctx, info)); }

  LinearStorageIndex findSame(const typename ItemType::CreationInfo &info)
  {
    uint32_t hash = info.getHash32();
    auto indexRange = itemsHashToIndex.equal_range(hash);
    auto ref = eastl::find_if(indexRange.first, indexRange.second, [&info, this](auto indexIter) {
      const auto &iter = items[indexIter.second];
      if (iter && !iter->isRemovalPending())
        return iter->isSame(info);

      return false;
    });

    return ref == indexRange.second ? items.size() : ref->second;
  }

  ItemID add(DeviceContext &ctx, const typename ItemType::CreationInfo &info)
  {
    LinearStorageIndex index = 0;
    if (!items.empty() && !ItemType::alwaysUnique())
    {
      index = findSame(info);
      if (index < items.size())
      {
        items[index]->onDuplicateAddition();
        return ItemType::makeID(index);
      }
    }

    if (freeIds.empty())
    {
      index = items.size();
      items.push_back();
    }
    else
    {
      index = freeIds.back();
      freeIds.pop_back();
    }

    auto id = ItemType::makeID(index);
    auto obj = eastl::make_unique<ItemType>(info);
    obj->addToContext(ctx, id, info);
    items[index] = eastl::move(obj);

    uint32_t hash = info.getHash32();
    itemsHashToIndex.emplace(hash, index);
    itemsIndexToHash.emplace(index, hash);

    return id;
  }

  ItemType *get(ItemID id)
  {
    G_ASSERT(ItemType::checkID(id));
    return items[ItemType::getIndexFromID(id)].get();
  }

  bool checkExistence(ItemID id)
  {
    LinearStorageIndex idx = ItemType::getIndexFromID(id);
    if (items.size() <= idx)
      return false;

    return items[idx].get() != nullptr;
  }

  template <typename T>
  void enumerate(T clb)
  {
    auto from = begin(items);
    auto to = end(items);
    for (auto at = from; at != to; ++at)
    {
      if (*at)
        clb(ItemType::makeID(at - from), **at);
    }
  }

private:
  eastl::vector<eastl::unique_ptr<ItemType>> items;
  eastl::vector<LinearStorageIndex> freeIds;
  eastl::vector_multimap<uint32_t, uint32_t> itemsHashToIndex;
  eastl::vector_map<uint32_t, uint32_t> itemsIndexToHash;
};

class ShaderProgramDatabase
{
public:
  ShaderProgramDatabase() = default;

  void init(bool has_bindless, DeviceContext &ctx);

  void shutdown()
  {
    WinAutoLock lock(dataGuard);

    progs.graphics.shutdown();
    progs.compute.shutdown();
    shaders.shutdown();
    shaderDesc.headers.shutdown();
    shaderDesc.modules.shutdown();
    shaderDesc.layouts.shutdown();
  }

  void clear(DeviceContext &ctx)
  {
    WinAutoLock lock(dataGuard);

    progs.graphics.clear(ctx);
    progs.compute.clear(ctx);
    shaders.clear(ctx);
    shaderDesc.headers.clear(ctx);
    shaderDesc.modules.clear(ctx);
    shaderDesc.layouts.clear(ctx);
  }

  // ----- programs

  void reuseId(ProgramID prog)
  {
    WinAutoLock lock(dataGuard);
    switch (get_program_type(prog))
    {
      case program_type_graphics: progs.graphics.reuseId(prog); return;
      case program_type_compute: progs.compute.reuseId(prog); return;
    }
  }

  void removeProgram(DeviceContext &ctx, ProgramID prog)
  {
    WinAutoLock lock(dataGuard);

    switch (get_program_type(prog))
    {
      case program_type_graphics: progs.graphics.remove(ctx, prog); return;
      case program_type_compute: progs.compute.remove(ctx, prog); return;
    }
  }

  ProgramID newComputeProgram(DeviceContext &ctx, const Tab<spirv::ChunkHeader> &chunks, const Tab<uint8_t> &chunk_data)
  {
    WinAutoLock lock(dataGuard);
    return progs.compute.add(ctx, {chunks, chunk_data});
  }

  ProgramID newGraphicsProgram(DeviceContext &ctx, InputLayoutID vdecl, ShaderID vs, ShaderID fs)
  {
    WinAutoLock lock(dataGuard);

    if (ShaderID::Null() == fs)
      fs = nullFragmentShader;

    return progs.graphics.add(ctx, {vdecl, shaders.get(vs), shaders.get(fs)});
  }

  InputLayoutID getGraphicsProgInputLayout(ProgramID id)
  {
    WinAutoLock lock(dataGuard);
    if (!progs.graphics.checkExistence(id))
      return InputLayoutID::Null();
    return progs.graphics.get(id)->inputLayout;
  }

  ProgramID getDebugProgram() const { return debugProgId; }

  // ----- shaders

  InputLayoutID registerInputLayout(DeviceContext &ctx, const InputLayout &layout)
  {
    WinAutoLock lock(dataGuard);
    return shaderDesc.layouts.add(ctx, layout);
  }

  InputLayoutID getInputLayout(DeviceContext &ctx, InputStreamSet vibd, const VertexAttributeSet &viad)
  {
    InputLayout newLayout;
    newLayout.streams = vibd;
    newLayout.attribs = viad;

    return registerInputLayout(ctx, newLayout);
  }

  InputLayout getInputLayoutFromId(InputLayoutID id)
  {
    if (id == InputLayoutID::Null())
      return InputLayout();

    WinAutoLock lock(dataGuard);
    return *shaderDesc.layouts.get(id);
  }

  ShaderID newShader(DeviceContext &ctx, VkShaderStageFlagBits stage, Tab<spirv::ChunkHeader> &chunks, Tab<uint8_t> &chunk_data);
  ShaderID newShader(DeviceContext &ctx, eastl::vector<VkShaderStageFlagBits> stage, eastl::vector<Tab<spirv::ChunkHeader>> chunks,
    eastl::vector<Tab<uint8_t>> chunk_data);

  void deleteShader(DeviceContext &ctx, ShaderID shader);
  ShaderID getNullFragmentShader() { return nullFragmentShader; }
  void setShaderDebugName(ShaderID shader, const char *name)
  {
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    WinAutoLock lock(dataGuard);
    // it is called after creation in same thread
    // should be fine to pass into debug info
    shaders.get(shader)->setDebugName(name);
#else
    G_UNUSED(shader);
    G_UNUSED(name);
#endif
  }

private:
  struct
  {
    ShaderProgramDatabaseStorage<GraphicsProgram, ProgramID> graphics;
    ShaderProgramDatabaseStorage<ComputeProgram, ProgramID> compute;
  } progs;

  struct
  {
    ShaderProgramDatabaseStorage<UniqueShaderHeader, int> headers;
    ShaderProgramDatabaseStorage<UniqueShaderModule, int> modules;
    ShaderProgramDatabaseStorage<InputLayout, InputLayoutID> layouts;
  } shaderDesc;

  ShaderProgramDatabaseStorage<ShaderInfo, ShaderID> shaders;

  struct CombinedChunkModules
  {
    struct Pair
    {
      Tab<spirv::ChunkHeader> *headers = nullptr;
      Tab<uint8_t> *data = nullptr;
    };

    Pair vs;
    Pair gs;
    Pair tc;
    Pair te;
  };

  bool extractShaderModules(const VkShaderStageFlagBits stage, const Tab<spirv::ChunkHeader> &chunk_header,
    const Tab<uint8_t> &chunk_data, ShaderModuleHeader &shader_header, ShaderModuleBlob &shader_blob);

  eastl::optional<ShaderInfo::CreationInfo> getShaderCreationInfo(DeviceContext &ctx, const CombinedChunkModules &modules);

  void initDebugProg(bool has_bindless, DeviceContext &dc);
  void initShaders(DeviceContext &ctx);

  ProgramID debugProgId;
  ShaderID nullFragmentShader;
  WinCritSec dataGuard;

  ShaderID newShader(DeviceContext &ctx, const ShaderModuleHeader &hdr, const ShaderModuleBlob &blob);
  void releaseShaderDeps(DeviceContext &ctx, ShaderInfo &item);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  void attachDebugInfo(ShaderID shader, const CombinedChunkModules &modules);
  void attachDebugInfo(ShaderID shader, const ShaderDebugInfo &debugInfo);
#endif
};

} // namespace drv3d_vulkan