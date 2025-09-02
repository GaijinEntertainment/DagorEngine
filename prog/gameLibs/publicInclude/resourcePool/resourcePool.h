//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_resizableTex.h>
#include <resourcePool/aliasableTex.h>
#include <generic/dag_objectPool.h>
#include <drv/3d/dag_info.h>

#include <ska_hash_map/flat_hash_map2.hpp>

#include <EASTL/array.h>
#include <EASTL/intrusive_ptr.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/stack.h>
#include <EASTL/tuple.h>
#include <EASTL/type_traits.h>

#include <stdio.h> // snprintf

enum class ResourceType
{
  Texture,
  CubeTexture,
  VolTexture,
  ArrayTexture,
  CubeArrayTexture,
  VBuffer,
  IBuffer,
  SBuffer,
  ResizableTexture,
  AliasableTexture,
  Last = AliasableTexture
};

namespace resource_pool_detail
{

static constexpr std::size_t MAX_RESOURCE_NAME_LENGTH = 32;
using ResourceName = eastl::array<char, MAX_RESOURCE_NAME_LENGTH>;

// disable aliased textures for a while due to various bugs
// TODO: fix aliased textures usage
static bool useAliasedTextures = false;

unsigned getNextUniqueResourceId(ResourceType type);

template <ResourceType T>
static ResourceName getUniqueResourceName(const char *basename)
{
  ResourceName result;
  snprintf(result.data(), result.size(), "%s_%d", basename, getNextUniqueResourceId(T));
  return result;
}

template <typename F>
struct FunctionTraits
{};

template <typename R, typename... Args>
struct FunctionTraits<R(Args...)>
{
  using ReturnValue = R;
  using Arguments = eastl::tuple<Args...>;
};

template <ResourceType>
struct ResourceFactory
{};

template <>
struct ResourceFactory<ResourceType::Texture>
{
  static UniqueTex create(int w, int h, int flg, int levels)
  {
    return dag::create_tex(nullptr, w, h, flg, levels, getUniqueResourceName<ResourceType::Texture>("tex").data());
  }

  static uint64_t get_size_for_alias(int, int, int, int) { return 0; }
  static void try_alias(UniqueTex &, int, int, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::CubeTexture>
{
  static UniqueTex create(int size, int flg, int levels)
  {
    return dag::create_cubetex(size, flg, levels, getUniqueResourceName<ResourceType::CubeTexture>("cubetex").data());
  }

  static uint64_t get_size_for_alias(int, int, int) { return 0; }
  static void try_alias(UniqueTex &, int, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::VolTexture>
{
  static UniqueTex create(int w, int h, int d, int flg, int levels)
  {
    return dag::create_voltex(w, h, d, flg, levels, getUniqueResourceName<ResourceType::VolTexture>("voltex").data());
  }

  static uint64_t get_size_for_alias(int, int, int, int, int) { return 0; }
  static void try_alias(UniqueTex &, int, int, int, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::ArrayTexture>
{
  static UniqueTex create(int w, int h, int d, int flg, int levels)
  {
    return dag::create_array_tex(w, h, d, flg, levels, getUniqueResourceName<ResourceType::ArrayTexture>("arraytex").data());
  }

  static uint64_t get_size_for_alias(int, int, int, int, int) { return 0; }
  static void try_alias(UniqueTex &, int, int, int, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::CubeArrayTexture>
{
  static UniqueTex create(int side, int d, int flg, int levels)
  {
    return dag::create_cube_array_tex(side, d, flg, levels,
      getUniqueResourceName<ResourceType::CubeArrayTexture>("cubearraytex").data());
  }

  static uint64_t get_size_for_alias(int, int, int, int) { return 0; }
  static void try_alias(UniqueTex &, int, int, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::VBuffer>
{
  static UniqueRes<ManagedRes<Vbuffer>> create(int sz, int f)
  {
    return dag::create_vb(sz, f, getUniqueResourceName<ResourceType::VBuffer>("vbuffer").data());
  }

  static uint64_t get_size_for_alias(int, int) { return 0; }
  static void try_alias(UniqueRes<ManagedRes<Vbuffer>> &, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::IBuffer>
{
  static UniqueRes<ManagedRes<Ibuffer>> create(int size_bytes, int flags)
  {
    return dag::create_ib(size_bytes, flags, getUniqueResourceName<ResourceType::IBuffer>("ibuffer").data());
  }

  static uint64_t get_size_for_alias(int, int) { return 0; }
  static void try_alias(UniqueRes<ManagedRes<Ibuffer>> &, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::SBuffer>
{
  static UniqueRes<ManagedRes<Sbuffer>> create(int struct_size, int elements, unsigned flags, unsigned texfmt)
  {
    return dag::create_sbuffer(struct_size, elements, flags, texfmt, getUniqueResourceName<ResourceType::SBuffer>("sbuffer").data());
  }

  static uint64_t get_size_for_alias(int, int, unsigned, unsigned) { return 0; }
  static void try_alias(UniqueRes<ManagedRes<Sbuffer>> &, int, int, unsigned, unsigned) {}
};

template <>
struct ResourceFactory<ResourceType::ResizableTexture>
{
  static ResizableTex create(int w, int h, int flg, int levels)
  {
    return dag::create_tex(nullptr, w, h, flg, levels, getUniqueResourceName<ResourceType::ResizableTexture>("restex").data());
  }

  static uint64_t get_size_for_alias(int, int, int, int) { return 0; }
  static void try_alias(ResizableTex &, int, int, int, int) {}
};

template <>
struct ResourceFactory<ResourceType::AliasableTexture>
{
  static AliasableTex2D create(int w, int h, int flg, int levels)
  {
    return dag::create_tex(nullptr, w, h, flg, levels, getUniqueResourceName<ResourceType::AliasableTexture>("aliastex").data());
  }

  static bool can_be_aliased(int flg)
  {
    // Render targets with DCC can't be aliased
    bool result = (d3d::get_driver_desc().caps.hasAliasedTextures && (flg & TEXCF_RT_COMPRESSED) == 0); // -V560 // -V616
    return (useAliasedTextures && result);
  }

  static uint64_t get_size_for_alias(int w, int h, int flg, int levels)
  {
    if (!can_be_aliased(flg))
      return 0;

    TextureResourceDescription texDesc{};
    texDesc.width = eastl::max(w, 1);
    texDesc.height = eastl::max(h, 1);
    texDesc.cFlags = flg & ~TEXCF_CLEAR_ON_CREATE;
    texDesc.mipLevels = levels;

    ResourceAllocationProperties allocProps = d3d::get_resource_allocation_properties(texDesc);
    return allocProps.sizeInBytes;
  }

  static void try_alias(AliasableTex2D &texture, int w, int h, int flg, int levels)
  {
    if (can_be_aliased(flg))
    {
      texture.alias(w, h, flg, levels);
    }
  }
};

template <typename T>
struct ResourceFactoryTraits
{
  using UniqueResType = typename FunctionTraits<decltype(T::create)>::ReturnValue;
  using Descriptor = typename FunctionTraits<decltype(T::create)>::Arguments;
};

template <typename TupleType>
struct TupleHash
{
  template <typename T>
  static void hash_combine(std::size_t &seed, const T &v)
  {
    eastl::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }

  std::size_t operator()(const TupleType &tuple) const
  {
    std::size_t seed = 0;
    eastl::apply(
      [&seed](const auto &...v) {
        // workaround to fold expression until C++17 lands...
        auto dummy = {(hash_combine(seed, v), 0)...};
        G_UNUSED(dummy);
      },
      tuple);
    return seed;
  }
};

} // namespace resource_pool_detail

template <ResourceType T>
class DescribedResourcePool
{
  using ResourceFactoryType = resource_pool_detail::ResourceFactory<T>;
  using ResourceTraitsType = resource_pool_detail::ResourceFactoryTraits<ResourceFactoryType>;
  using DescriptorType = typename ResourceTraitsType::Descriptor;
  using UniqueResType = typename ResourceTraitsType::UniqueResType;
  using WPtr = eastl::weak_ptr<DescribedResourcePool>;

public:
  using Ptr = eastl::shared_ptr<DescribedResourcePool>;
  using CPtr = eastl::shared_ptr<const DescribedResourcePool>;

  struct ResourcePool;

  class LeasedResource : public UniqueResType
  {
  public:
    using Ptr = eastl::intrusive_ptr<LeasedResource>;
    using CPtr = eastl::intrusive_ptr<const LeasedResource>;

    LeasedResource(const DescriptorType &desc, ResourcePool &owner) :
      UniqueResType(eastl::apply(resource_pool_detail::ResourceFactory<T>::create, desc)), owner(owner)
    {}

    ~LeasedResource() { G_ASSERT(!useCount); }

    // disable moving from/to a leased resource
    LeasedResource(LeasedResource &&) = delete;
    LeasedResource &operator=(LeasedResource &&) = delete;

    void updateLastUsedFrame(unsigned int frameIdx) { lastUsedFrame = frameIdx; }
    unsigned int getLastUsedFrame() const { return lastUsedFrame; }

  private:
    template <typename U>
    friend void eastl::intrusive_ptr_add_ref(U *);
    template <typename U>
    friend void eastl::intrusive_ptr_release(U *);

    // NOT thread safe
    void AddRef() const { ++useCount; }

    void Release() const
    {
      if (!(--useCount))
        owner.release(const_cast<LeasedResource &>(*this));
    }

    ResourcePool &owner;
    mutable unsigned useCount = 0;

    unsigned int lastUsedFrame = 0;
  };

  struct ResourcePool
  {
    eastl::vector<LeasedResource *> freeList;
    ObjectPool<LeasedResource, OBJECT_POOL_DEFAULT_BLOCK_SIZE, ObjectPoolPolicyRandomAllocationAndFree> pool;

    auto acquire(const DescriptorType &descriptor)
    {
      typename LeasedResource::Ptr result;

      if (freeList.empty())
      {
        result = pool.allocate(descriptor, *this);
      }
      else
      {
        result = freeList.back();
        freeList.pop_back();
        eastl::apply(resource_pool_detail::ResourceFactory<T>::try_alias, eastl::tuple_cat(eastl::tie(*result), descriptor));
      }

      result->updateLastUsedFrame(::dagor_frame_no());

      return result;
    }

    void release(LeasedResource &resource)
    {
      G_ASSERT(resource);
      resource.updateLastUsedFrame(::dagor_frame_no());
      freeList.emplace_back(&resource);
    }
  };

  using ResourcePoolPtr = eastl::shared_ptr<ResourcePool>;
  using ResourcePoolWPtr = eastl::weak_ptr<ResourcePool>;

  static inline ska::flat_hash_map<uint64_t, ResourcePoolWPtr> &getResourcePoolMap() // Used only for resources with aliasing support
  {
    static ska::flat_hash_map<uint64_t, ResourcePoolWPtr> resourcePoolMap = {};
    return resourcePoolMap;
  }
  static inline ska::flat_hash_map<DescriptorType, WPtr, resource_pool_detail::TupleHash<DescriptorType>> &getDescribedPoolMap()
  {
    static ska::flat_hash_map<DescriptorType, WPtr, resource_pool_detail::TupleHash<DescriptorType>> describedPoolMap = {};
    return describedPoolMap;
  }

  DescribedResourcePool(const DescriptorType &descriptor) : resourceDescriptor(descriptor) {}

  const DescriptorType &getDescriptor() const { return resourceDescriptor; }

  template <typename... CreateArgs>
  static Ptr get(CreateArgs &&...args)
  {
    using namespace resource_pool_detail;

    Ptr result;

    DescriptorType desc{args...};

    auto &describedPoolMap = getDescribedPoolMap();
    auto it = describedPoolMap.find(desc);
    if (it == describedPoolMap.end() || it->second.expired())
    {
      result = eastl::make_shared<DescribedResourcePool>(desc);

      ResourcePoolPtr newResPool;

      uint64_t bytesForAlias = eastl::apply(resource_pool_detail::ResourceFactory<T>::get_size_for_alias, desc);
      if (bytesForAlias > 0)
      {
        auto &resourcePoolMap = getResourcePoolMap();
        auto itAlias = resourcePoolMap.find(bytesForAlias);
        if (itAlias == resourcePoolMap.end() || itAlias->second.expired())
        {
          newResPool = eastl::make_shared<ResourcePool>();
          resourcePoolMap.insert_or_assign(bytesForAlias, newResPool);
        }
        else
        {
          newResPool = itAlias->second.lock();
        }
      }
      else
      {
        newResPool = eastl::make_shared<ResourcePool>();
      }

      result->resourcePool = newResPool;
      describedPoolMap.insert_or_assign(desc, result);
    }
    else
    {
      result = it->second.lock();
    }

    return result;
  }

  auto acquire()
  {
    typename LeasedResource::Ptr result;

    G_ASSERT(resourcePool);
    result = resourcePool->acquire(getDescriptor());

    return result;
  }

  static void freeUnused(unsigned int frameCountForFree)
  {
    unsigned int curFrame = ::dagor_frame_no();
    for (auto &curDescribedPool : getDescribedPoolMap())
    {
      auto poolWithDescPtr = curDescribedPool.second.lock();
      if (poolWithDescPtr && !poolWithDescPtr->resourcePool->freeList.empty())
      {
        auto &vec = poolWithDescPtr->resourcePool->freeList;
        uint32_t curIndex = 0, lastIndex = vec.size();
        while (curIndex < lastIndex)
        {
          if (curFrame - vec[curIndex]->getLastUsedFrame() > frameCountForFree)
          {
            lastIndex--;
            if (curIndex != lastIndex)
            {
              eastl::swap(vec[curIndex], vec[lastIndex]);
            }
          }
          else
            curIndex++;
        }

        if (lastIndex != vec.size())
        {
          for (uint32_t i = lastIndex; i < vec.size(); i++)
          {
            poolWithDescPtr->resourcePool->pool.free(vec[i]);
          }
          vec.resize(lastIndex);
        }
      }
    }
  }

private:
  DescriptorType resourceDescriptor;
  ResourcePoolPtr resourcePool;
};

using RTargetPool = DescribedResourcePool<ResourceType::AliasableTexture>;
using RTarget = RTargetPool::LeasedResource;

using ResizableRTargetPool = DescribedResourcePool<ResourceType::AliasableTexture>;
using ResizableRTarget = ResizableRTargetPool::LeasedResource;

static inline int get_width(const RTargetPool &pool) { return eastl::get<0>(pool.getDescriptor()); }

static inline int get_height(const RTargetPool &pool) { return eastl::get<1>(pool.getDescriptor()); }

static inline int get_format(const RTargetPool &pool) { return eastl::get<2>(pool.getDescriptor()); }

static inline int get_levels(const RTargetPool &pool) { return eastl::get<3>(pool.getDescriptor()); }
