//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_resizableTex.h>
#include <generic/dag_objectPool.h>

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
  Last = ResizableTexture
};

namespace resource_pool_detail
{

static constexpr std::size_t MAX_RESOURCE_NAME_LENGTH = 32;
using ResourceName = eastl::array<char, MAX_RESOURCE_NAME_LENGTH>;

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

  static void reset(UniqueTex &texture)
  {
    texture->texaddr(TEXADDR_CLAMP);
    texture->texfilter(TEXFILTER_DEFAULT);
    texture->texbordercolor(0);
    texture->texmipmap(TEXMIPMAP_DEFAULT);
  }
};

template <>
struct ResourceFactory<ResourceType::CubeTexture>
{
  static UniqueTex create(int size, int flg, int levels)
  {
    return dag::create_cubetex(size, flg, levels, getUniqueResourceName<ResourceType::CubeTexture>("cubetex").data());
  }

  static void reset(UniqueTex &texture)
  {
    texture->texaddr(TEXADDR_CLAMP);
    texture->texfilter(TEXFILTER_DEFAULT);
    texture->texbordercolor(0);
    texture->texmipmap(TEXMIPMAP_DEFAULT);
  }
};

template <>
struct ResourceFactory<ResourceType::VolTexture>
{
  static UniqueTex create(int w, int h, int d, int flg, int levels)
  {
    return dag::create_voltex(w, h, d, flg, levels, getUniqueResourceName<ResourceType::VolTexture>("voltex").data());
  }

  static void reset(UniqueTex &texture)
  {
    texture->texaddr(TEXADDR_CLAMP);
    texture->texfilter(TEXFILTER_DEFAULT);
    texture->texbordercolor(0);
    texture->texmipmap(TEXMIPMAP_DEFAULT);
  }
};

template <>
struct ResourceFactory<ResourceType::ArrayTexture>
{
  static UniqueTex create(int w, int h, int d, int flg, int levels)
  {
    return dag::create_array_tex(w, h, d, flg, levels, getUniqueResourceName<ResourceType::ArrayTexture>("arraytex").data());
  }

  static void reset(UniqueTex &texture)
  {
    texture->texaddr(TEXADDR_CLAMP);
    texture->texfilter(TEXFILTER_DEFAULT);
    texture->texbordercolor(0);
    texture->texmipmap(TEXMIPMAP_DEFAULT);
  }
};

template <>
struct ResourceFactory<ResourceType::CubeArrayTexture>
{
  static UniqueTex create(int side, int d, int flg, int levels)
  {
    return dag::create_cube_array_tex(side, d, flg, levels,
      getUniqueResourceName<ResourceType::CubeArrayTexture>("cubearraytex").data());
  }

  static void reset(UniqueTex &texture)
  {
    texture->texaddr(TEXADDR_CLAMP);
    texture->texfilter(TEXFILTER_DEFAULT);
    texture->texbordercolor(0);
    texture->texmipmap(TEXMIPMAP_DEFAULT);
  }
};

template <>
struct ResourceFactory<ResourceType::VBuffer>
{
  static UniqueRes<ManagedRes<Vbuffer>> create(int sz, int f)
  {
    return dag::create_vb(sz, f, getUniqueResourceName<ResourceType::VBuffer>("vbuffer").data());
  }

  static void reset(UniqueRes<ManagedRes<Vbuffer>> &) {}
};

template <>
struct ResourceFactory<ResourceType::IBuffer>
{
  static UniqueRes<ManagedRes<Ibuffer>> create(int size_bytes, int flags)
  {
    return dag::create_ib(size_bytes, flags, getUniqueResourceName<ResourceType::IBuffer>("ibuffer").data());
  }

  static void reset(UniqueRes<ManagedRes<Ibuffer>> &) {}
};

template <>
struct ResourceFactory<ResourceType::SBuffer>
{
  static UniqueRes<ManagedRes<Sbuffer>> create(int struct_size, int elements, unsigned flags, unsigned texfmt)
  {
    return dag::create_sbuffer(struct_size, elements, flags, texfmt, getUniqueResourceName<ResourceType::SBuffer>("sbuffer").data());
  }

  static void reset(UniqueRes<ManagedRes<Sbuffer>> &) {}
};

template <>
struct ResourceFactory<ResourceType::ResizableTexture>
{
  static ResizableTex create(int w, int h, int flg, int levels)
  {
    return dag::create_tex(nullptr, w, h, flg, levels, getUniqueResourceName<ResourceType::ResizableTexture>("restex").data());
  }

  static void reset(ResizableTex &texture)
  {
    texture->texaddr(TEXADDR_CLAMP);
    texture->texfilter(TEXFILTER_DEFAULT);
    texture->texbordercolor(0);
    texture->texmipmap(TEXMIPMAP_DEFAULT);
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
class ResourcePool
{
  using ResourceFactoryType = resource_pool_detail::ResourceFactory<T>;
  using ResourceTraitsType = resource_pool_detail::ResourceFactoryTraits<ResourceFactoryType>;
  using DescriptorType = typename ResourceTraitsType::Descriptor;
  using UniqueResType = typename ResourceTraitsType::UniqueResType;
  using WPtr = eastl::weak_ptr<ResourcePool>;

public:
  using Ptr = eastl::shared_ptr<ResourcePool>;
  using CPtr = eastl::shared_ptr<const ResourcePool>;

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
  };

  ResourcePool(const DescriptorType &descriptor) : descriptor(descriptor){};

  const DescriptorType &getDescriptor() const { return descriptor; }

  template <typename... CreateArgs>
  static Ptr get(CreateArgs &&...args)
  {
    using namespace resource_pool_detail;

    static ska::flat_hash_map<DescriptorType, WPtr, TupleHash<DescriptorType>> pools;
    Ptr result;

    DescriptorType desc{args...};

    auto it = pools.find(desc);
    if (it == pools.end() || it->second.expired())
    {
      result = eastl::make_shared<ResourcePool>(desc);
      pools.insert(it, {desc, result});
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

    if (freeList.empty())
    {
      result = pool.allocate(descriptor, *this);
    }
    else
    {
      result = freeList.top();
      freeList.pop();
    }

    return result;
  }

  void release(LeasedResource &resource)
  {
    G_ASSERT(resource);
    resource_pool_detail::ResourceFactory<T>::reset(resource);
    freeList.emplace_back(&resource);
  }

private:
  DescriptorType descriptor;
  eastl::stack<LeasedResource *> freeList;
  ObjectPool<LeasedResource, OBJECT_POOL_DEFAULT_BLOCK_SIZE, ObjectPoolPolicyLinearAllocationAndNoFree> pool;
};

using RTargetPool = ResourcePool<ResourceType::Texture>;
using RTarget = RTargetPool::LeasedResource;

using ResizableRTargetPool = ResourcePool<ResourceType::ResizableTexture>;
using ResizableRTarget = ResizableRTargetPool::LeasedResource;

static inline int get_width(const RTargetPool &pool) { return eastl::get<0>(pool.getDescriptor()); }

static inline int get_height(const RTargetPool &pool) { return eastl::get<1>(pool.getDescriptor()); }

static inline int get_width(const ResizableRTargetPool &pool) { return eastl::get<0>(pool.getDescriptor()); }

static inline int get_height(const ResizableRTargetPool &pool) { return eastl::get<1>(pool.getDescriptor()); }
