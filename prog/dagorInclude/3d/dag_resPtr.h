//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_resMgr.h"
#include "dag_texMgr.h"
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_buffers.h>
#include <shaders/dag_shaders.h>
#include <gameRes/dag_gameResources.h>
#include <debug/dag_assert.h>
#include <EASTL/algorithm.h>
#include <EASTL/type_traits.h>
#include <util/dag_preprocessor.h>

#if DAGOR_DBGLEVEL > 1
#include <EASTL/unordered_set.h>
#endif

/* This header contains several useful RAII classes for working with resources

   -------------------       --------------------       -------------------
   |    UniqueRes    |------>|    ManagedRes    |<------|    SharedRes    |
   -------------------       --------------------       -------------------
                                      ^
                                      |
   -------------------       --------------------       -------------------
   | UniqueResHolder |------>| ManagedResHolder |<------| SharedResHolder |
   -------------------       --------------------       -------------------

  The `ManagedRes` owns a pointer to the resource and a registered resource id.
  And the `ManagedResHolder` also contains the shader variable id. You cannot
  create an instance for `ManagedRes` and `ManagedResHolder`. They don't know
  anything about how exactly ownership of the resource was obtained and cannot
  release it. These classes can be used to pass to functions by const reference

  The `UniqueRes` and `UniqueResHolder` classes are needed for the unique
  ownership of the resource and can be created using the family of functions
  `dag::create_tex()`, For example:

  UniqueTex tex = dag::create_tex(..., "tex_name");

  "tex_name" will be used both for the texture name and for registration in
  the resource manager. If you need a texture name other than the name of the
  registration in the manager, then you can use this:

  UniqueTex tex = UniqueTex(dag::create_tex(..., "tex_name"), "mgr_name");

  The `UniqueResHolder` is created in the same way as the `UniqueRes`:

  UniqueResHolder tex = dag::create_tex(..., "tex_name");

  In this case, the "tex_name" will also be used as a shader variable name, but
  if you need a shader variable name other than the texture name, then use this:

  UniqueResHolder tex = UniqueResHolder(dag::create_tex(..., "tex_name"), "var_name");

  If you need to bind a resource to another shader variable, then use this:

  tex.setVar("another_var_name");

  The `SharedRes` and `SharedResHolder` are needed for shared ownership of the
  resource. For example, they can be used to own game resources:

  SharedTex res = dag::get_tex_gameres("res_name");
  SharedTexHolder res_holder = dag::get_tex_gameres("res_name", "var_name");
  SharedTexHolder other_res_holder(eastl::move(res), "var_name");
*/

namespace resptr_detail
{

template <typename ResType>
struct Helper
{
  using resid_t = D3DRESID;
  static constexpr resid_t BAD_ID = D3DRESID(D3DRESID::INVALID_ID);
  static resid_t registerRes(const char *name, ResType *res) { return register_managed_res(name, res); }
  static ResType *acquire(resid_t resId) { return acquire_managed_res(resId); }
  static void release(resid_t &resId, ResType *res) { return release_managed_res_verified(resId, res); }
  static bool isManagedFactorySet(resid_t) { return false; }
  static int getManagedRefCount(resid_t resId) { return get_managed_res_refcount(resId); }
  static const char *getName(ResType *) { return ""; }
  static const char *getNameById(resid_t resId) { return get_managed_res_name(resId); }
  static bool setVar(int, resid_t) { return false; }
  static bool can_bind(ResType *) { return false; }
};

template <>
struct Helper<Sbuffer> : public Helper<D3dResource>
{
  static Sbuffer *acquire(resid_t resId) { return acquire_managed_buf(resId); }
  static void release(resid_t &resId, Sbuffer *res) { return release_managed_buf_verified(resId, res); }
  static const char *getName(Sbuffer *res) { return res ? res->getBufName() : nullptr; }
  static bool setVar(int varId, resid_t resId) { return ShaderGlobal::set_buffer(varId, resId); }
  static constexpr uint32_t valid_use_mask = SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED | SBCF_BIND_CONSTANT;
  static bool can_bind(Sbuffer *res) { return !res || (res->getFlags() & valid_use_mask); }
};

template <>
struct Helper<BaseTexture>
{
  using resid_t = TEXTUREID;
  static constexpr resid_t BAD_ID = D3DRESID(D3DRESID::INVALID_ID);
  static resid_t registerRes(const char *name, BaseTexture *res) { return register_managed_tex(name, res); }
  static BaseTexture *acquire(resid_t resId) { return acquire_managed_tex(resId); }
  static void release(resid_t &resId, BaseTexture *res) { return release_managed_tex_verified(resId, res); }
  static bool isManagedFactorySet(resid_t resId) { return is_managed_tex_factory_set(resId); }
  static int getManagedRefCount(resid_t resId) { return get_managed_texture_refcount(resId); }
  static const char *getName(BaseTexture *res) { return res ? res->getTexName() : nullptr; }
  static const char *getNameById(resid_t resId) { return get_managed_texture_name(resId); }
  static bool setVar(int varId, resid_t resId) { return ShaderGlobal::set_texture(varId, resId); }
  static bool can_bind(BaseTexture *) { return true; }
};

struct ExternalTexWrapper
{};

template <>
struct Helper<ExternalTexWrapper> : Helper<BaseTexture>
{
  static bool release(resid_t &res_id, BaseTexture *)
  {
    if (res_id == BAD_ID)
      return true;
    if (!change_managed_texture(res_id, nullptr))
      return false;
    release_managed_tex(res_id);
    return true;
  }
};

static inline bool isVoltex(D3dResource *res) { return !res || res->restype() == RES3D_VOLTEX; }
static inline bool isArrtex(D3dResource *res) { return !res || res->restype() == RES3D_ARRTEX || res->restype() == RES3D_CUBEARRTEX; }
static inline bool isCubetex(D3dResource *res)
{
  return !res || res->restype() == RES3D_CUBETEX || res->restype() == RES3D_CUBEARRTEX;
}
static inline bool isTex2d(D3dResource *res) { return !res || res->restype() == RES3D_TEX; }
static inline bool isBuf(D3dResource *res) { return !res || res->restype() == RES3D_SBUF; }

struct PrivateDataGetter
{
  template <typename T>
  static typename T::derived_t::resource_t *getResource(const T *that)
  {
    return static_cast<const typename T::derived_t *>(that)->mResource;
  }

  template <typename T>
  static typename T::derived_t::resid_t getResId(const T *that)
  {
    return static_cast<const typename T::derived_t *>(that)->mResId;
  }
};

template <typename DerivedType>
class TextureGetter
{
public:
  using derived_t = DerivedType;

  BaseTexture *getBaseTex() const { return PrivateDataGetter::getResource(this); }
  Texture *getTex2D() const
  {
    return isTex2d(PrivateDataGetter::getResource(this)) ? (Texture *)PrivateDataGetter::getResource(this) : nullptr;
  }
  ArrayTexture *getArrayTex() const
  {
    return isArrtex(PrivateDataGetter::getResource(this)) ? (ArrayTexture *)PrivateDataGetter::getResource(this) : nullptr;
  }
  CubeTexture *getCubeTex() const
  {
    return isCubetex(PrivateDataGetter::getResource(this)) ? (CubeTexture *)PrivateDataGetter::getResource(this) : nullptr;
  }
  VolTexture *getVolTex() const
  {
    return isVoltex(PrivateDataGetter::getResource(this)) ? (VolTexture *)PrivateDataGetter::getResource(this) : nullptr;
  }
  TEXTUREID getTexId() const { return PrivateDataGetter::getResId(this); }
};

template <typename DerivedType>
class BufferGetter
{
public:
  using derived_t = DerivedType;

  Sbuffer *getBuf() const
  {
    G_ASSERT(isBuf(PrivateDataGetter::getResource(this)));
    return (Sbuffer *)PrivateDataGetter::getResource(this);
  }
  D3DRESID getBufId() const { return PrivateDataGetter::getResId(this); }
};

template <typename Type, typename ResType>
using ResourceGetter = eastl::conditional_t<eastl::is_same_v<ResType, Sbuffer>, BufferGetter<Type>, TextureGetter<Type>>;

class MoveAssignable
{
protected:
  template <typename Type>
  static bool isValidForMoveAssignment(const Type &, const Type &)
  {
    return true;
  }
};

class WeakMoveAssignable
{
protected:
  template <typename Type>
  static bool isValidForMoveAssignment(const Type &left, const Type &right)
  {
    return !left || (PrivateDataGetter::getResId(&left) != PrivateDataGetter::getResId(&right));
  }
};

#define ENABLE_MOVING_BEHAVIOR(Class)                                                                                  \
  Class() = default;                                                                                                   \
  Class(Class &&other) noexcept                                                                                        \
  {                                                                                                                    \
    this->swap(other);                                                                                                 \
  }                                                                                                                    \
  Class &operator=(Class &&other)                                                                                      \
  {                                                                                                                    \
    G_ASSERTF(Class::isValidForMoveAssignment(*this, other), "Invalid for move assignment. Call close() before this"); \
    Class(eastl::move(other)).swap(*this);                                                                             \
    return *this;                                                                                                      \
  }                                                                                                                    \
  void close()                                                                                                         \
  {                                                                                                                    \
    *this = Class();                                                                                                   \
  }

#define ASSIGNMENT_FROM_RESPTR(Class)       \
  Class &operator=(ResPtr<ResType> &&res)   \
  {                                         \
    close();                                \
    return *this = Class(eastl::move(res)); \
  }

template <typename ResType>
class ResPtr : public MoveAssignable
{
  ResType *mResource = nullptr;

  void swap(ResPtr &other) { eastl::swap(mResource, other.mResource); }

  ResPtr(ResType *res) : mResource(res) {}

  template <typename Type>
  friend ResPtr<Type> ResPtrFactory(Type *res);

public:
  ENABLE_MOVING_BEHAVIOR(ResPtr);

  ~ResPtr()
  {
    if (mResource)
      mResource->destroy();
  }

  ResType *release() &&
  {
    ResType *resource = mResource;
    mResource = nullptr;
    return resource;
  }

  ResType *get() const & { return mResource; }
  ResType *operator->() const & { return mResource; }
  explicit operator bool() const & { return mResource; }
};

template <typename ResType>
static inline ResPtr<ResType> ResPtrFactory(ResType *res)
{
  return {res};
}

template <typename ResType>
class ManagedRes : public ResourceGetter<ManagedRes<ResType>, ResType>
{
public:
  using resource_t = ResType;
  using resid_t = typename Helper<ResType>::resid_t;

protected:
  friend PrivateDataGetter;

  resid_t mResId = Helper<ResType>::BAD_ID;
  ResType *mResource = nullptr;

  void swap(ManagedRes &other)
  {
    eastl::swap(mResId, other.mResId);
    eastl::swap(mResource, other.mResource);
  }

  ManagedRes() = default;

public:
  ResType *operator->() const { return mResource; }

  explicit operator bool() const { return mResource && mResId != Helper<ResType>::BAD_ID; }
};

template <typename ManagedResType, typename Deleter = Helper<typename ManagedResType::resource_t>>
class UniqueRes : public ManagedResType, public WeakMoveAssignable
{
public:
  using move_assignable_t = WeakMoveAssignable;
  using BaseType = ManagedResType;
  using ResType = typename BaseType::resource_t;

  ENABLE_MOVING_BEHAVIOR(UniqueRes);
  ASSIGNMENT_FROM_RESPTR(UniqueRes);

  UniqueRes(ResPtr<ResType> &&res, const char *managed_name = nullptr)
  {
    if (!managed_name)
      managed_name = Helper<ResType>::getName(res.get());

    this->mResId = Helper<ResType>::registerRes(managed_name, res.get());
    this->mResource = eastl::move(res).release();
  }

  ~UniqueRes() { release(this->mResId, this->mResource); }

  static void release(typename Helper<ResType>::resid_t &res_id, ResType *res)
  {
    if (Helper<ResType>::getManagedRefCount(res_id) > 1)
      ShaderGlobal::reset_from_vars(res_id);
    G_ASSERTF(res_id == Helper<ResType>::BAD_ID || Helper<ResType>::getManagedRefCount(res_id) == 1,
      "resource_name = '%s' res_id=0x%x refCount=%d", res ? res->getResName() : "", res_id,
      Helper<ResType>::getManagedRefCount(res_id));
    Deleter::release(res_id, res);
  }
};

template <typename ManagedResType, typename Deleter = Helper<typename ManagedResType::resource_t>>
class SharedRes : public ManagedResType, public MoveAssignable
{
public:
  using move_assignable_t = MoveAssignable;
  using BaseType = ManagedResType;
  using ResType = typename BaseType::resource_t;

  ENABLE_MOVING_BEHAVIOR(SharedRes);

  explicit SharedRes(typename Helper<ResType>::resid_t res_id)
  {
    this->mResId = res_id;
    this->mResource = Helper<ResType>::acquire(res_id);
  }

  explicit SharedRes(ResPtr<ResType> &&res)
  {
    this->mResource = eastl::move(res).release();
    this->mResId = Helper<ResType>::registerRes(Helper<ResType>::getName(this->mResource), this->mResource);
  }

  SharedRes(const SharedRes &other) : SharedRes(other.mResId) {}

  SharedRes &operator=(const SharedRes &other)
  {
    SharedRes(other).swap(*this);
    return *this;
  }

  ~SharedRes() { release(this->mResId, this->mResource); }

  static void release(typename Helper<ResType>::resid_t &res_id, ResType *res)
  {
    Deleter::release(res_id, res);
    if (Helper<ResType>::getManagedRefCount(res_id) >= 0)
      evict_managed_tex_id(res_id);
  }
};

template <typename ManagedResType>
class ManagedResView : public ManagedResType
{
public:
  using BaseType = ManagedResType;
  using ResType = typename BaseType::resource_t;

  ManagedResView() = default;
  ManagedResView(const ManagedResType &res)
  {
    this->mResId = PrivateDataGetter::getResId(&res);
    this->mResource = PrivateDataGetter::getResource(&res);
  }
  ManagedResView(ManagedResType &&res) = delete;
};

#if DAGOR_DBGLEVEL > 1
// This is a trick to create a global variable in the header.
// Despite the fact that this header will be included to different cpp,
// the linker will leave only one copy of this variable
template <typename T = void>
eastl::unordered_set<int> gUsedShaderVarIds;

static void acquireShaderVarId(int new_id)
{
  if (new_id != VariableMap::BAD_ID && !gUsedShaderVarIds<>.emplace(new_id).second)
    DAG_FATAL("You are trying to bind resource to a shaderVar with id = %d"
              " to which another resource is already bound, this can lead to unpredictable bugs",
      new_id);
}

static void releaseShaderVarId(int id)
{
  if (id == VariableMap::BAD_ID)
    return;

  size_t count = gUsedShaderVarIds<>.erase(id);
  G_ASSERT(count > 0);
}
#else
static void acquireShaderVarId(int) {}
static void releaseShaderVarId(int) {}
#endif

template <typename ManagedResType>
class ManagedResHolder : public ManagedResType
{
  using ResType = typename ManagedResType::resource_t;

protected:
  int mShaderVarId = -1;

  void swap(ManagedResHolder &other)
  {
    ManagedResType::swap(other);
    eastl::swap(mShaderVarId, other.mShaderVarId);
  }

  ManagedResHolder() = default;

public:
  int getVarId() const { return mShaderVarId; }

  void setVar() const
  {
    if (mShaderVarId != -1)
      Helper<ResType>::setVar(mShaderVarId, this->mResId);
  }

  void setVarId(int var_id)
  {
    releaseShaderVarId(mShaderVarId);
    acquireShaderVarId(var_id);
    mShaderVarId = var_id;
  }
};

template <typename ManagedResType>
class ConcreteResHolder : public ManagedResHolder<typename ManagedResType::BaseType>, public ManagedResType::move_assignable_t
{
  using ResType = typename ManagedResType::resource_t;

public:
  ENABLE_MOVING_BEHAVIOR(ConcreteResHolder);
  ASSIGNMENT_FROM_RESPTR(ConcreteResHolder);

  ConcreteResHolder(ManagedResType &&resource, const char *shader_var_name = nullptr)
  {
    ManagedResType::BaseType::swap(resource);

    if (this->mResId == Helper<ResType>::BAD_ID)
    {
      const char *errString = "ConcreteResHolder constructor failed. mResId is equal to BAD_ID";
      if (!shader_var_name)
      {
        logerr(errString);
        return;
      }
      logwarn("%s (shader_var_name: %s)", errString, shader_var_name);
    }

    if (!shader_var_name)
      shader_var_name = Helper<ResType>::getNameById(this->mResId);

    this->setVarId(get_shader_variable_id(shader_var_name));
    if (Helper<ResType>::can_bind(this->mResource))
      this->setVar();
  }

  ConcreteResHolder(ManagedResType &&resource, int var_id)
  {
    ManagedResType::BaseType::swap(resource);

    this->setVarId(var_id);
    if (Helper<ResType>::can_bind(this->mResource))
      this->setVar();
  }

  ConcreteResHolder(ResPtr<ResType> &&res, const char *shader_var_name = nullptr) :
    ConcreteResHolder(ManagedResType(eastl::move(res)), shader_var_name)
  {}

  ~ConcreteResHolder()
  {
    releaseShaderVarId(this->mShaderVarId);
    ManagedResType::release(this->mResId, this->mResource);
    if (this->mShaderVarId != -1)
      Helper<ResType>::setVar(this->mShaderVarId, Helper<ResType>::BAD_ID);
  }
};

#undef ENABLE_MOVING_BEHAVIOR
#undef ASSIGNMENT_FROM_RESPTR

} // namespace resptr_detail

template <typename ResType>
using ManagedRes = resptr_detail::ManagedRes<ResType>;
using ManagedTex = ManagedRes<BaseTexture>;
using ManagedBuf = ManagedRes<Sbuffer>;

template <typename ManagedResType>
using UniqueRes = resptr_detail::UniqueRes<ManagedResType>;
using UniqueTex = UniqueRes<ManagedTex>;
using UniqueBuf = UniqueRes<ManagedBuf>;

using ExternalTex = resptr_detail::UniqueRes<ManagedTex, resptr_detail::Helper<resptr_detail::ExternalTexWrapper>>;
// implement change_managed_buf for ExternalBuf

template <typename ManagedResType>
using ManagedResView = resptr_detail::ManagedResView<ManagedResType>;
using ManagedTexView = ManagedResView<ManagedTex>;
using ManagedBufView = ManagedResView<ManagedBuf>;

template <typename ManagedResType>
using SharedRes = resptr_detail::SharedRes<ManagedResType>;
using SharedTex = SharedRes<ManagedTex>;
using SharedBuf = SharedRes<ManagedBuf>;

using ManagedTexHolder = resptr_detail::ManagedResHolder<ManagedTex>;
using ManagedBufHolder = resptr_detail::ManagedResHolder<ManagedBuf>;
using UniqueTexHolder = resptr_detail::ConcreteResHolder<UniqueTex>;
using UniqueBufHolder = resptr_detail::ConcreteResHolder<UniqueBuf>;
using SharedTexHolder = resptr_detail::ConcreteResHolder<SharedTex>;
using SharedBufHolder = resptr_detail::ConcreteResHolder<SharedBuf>;

template <typename ResType>
using ResPtr = resptr_detail::ResPtr<ResType>;
using TexPtr = ResPtr<BaseTexture>;
using BufPtr = ResPtr<Sbuffer>;

namespace dag
{

static inline ResPtr<BaseTexture> create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_tex(img, w, h, flg, levels, name));
}
static inline ResPtr<CubeTexture> create_cubetex(int size, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_cubetex(size, flg, levels, name));
}
static inline ResPtr<VolTexture> create_voltex(int w, int h, int d, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_voltex(w, h, d, flg, levels, name));
}
static inline ResPtr<ArrayTexture> create_array_tex(int w, int h, int d, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_array_tex(w, h, d, flg, levels, name));
}
static inline ResPtr<ArrayTexture> create_cube_array_tex(int side, int d, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_cube_array_tex(side, d, flg, levels, name));
}
static inline TexPtr create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels = 0, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::create_ddsx_tex(crd, flg, quality_id, levels, name));
}
static inline TexPtr alias_tex(BaseTexture *base_tex, TexImage32 *img, int w, int h, int flg, int levels, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::alias_tex(base_tex, img, w, h, flg, levels, name));
}
static inline TexPtr alias_cubetex(BaseTexture *base_tex, int side, int flg, int levels, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::alias_cubetex(base_tex, side, flg, levels, name));
}
static inline TexPtr alias_voltex(BaseTexture *base_tex, int w, int h, int d, int flg, int levels, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::alias_voltex(base_tex, w, h, d, flg, levels, name));
}
static inline TexPtr alias_array_tex(BaseTexture *base_tex, int w, int h, int d, int flg, int levels, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::alias_array_tex(base_tex, w, h, d, flg, levels, name));
}
static inline TexPtr alias_cube_array_tex(BaseTexture *base_tex, int side, int d, int flg, int levels, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::alias_cube_array_tex(base_tex, side, d, flg, levels, name));
}

static inline ResPtr<Vbuffer> create_vb(int sz, int f, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_vb(sz, f, name));
}
static inline ResPtr<Ibuffer> create_ib(int size_bytes, int flags, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_ib(size_bytes, flags, name));
}
static inline ResPtr<Sbuffer> create_sbuffer(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_sbuffer(struct_size, elements, flags, texfmt, name));
}

namespace buffers
{

using namespace d3d::buffers;

// Such buffers could be updated from time to time and its content will be automatically restored on device reset.
inline ResPtr<Sbuffer> create_persistent_cb(uint32_t registers_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_persistent_cb(registers_count, name));
}
// Such buffers must be updated every frame. Because of that we don't care about its content on device reset.
inline ResPtr<Sbuffer> create_one_frame_cb(uint32_t registers_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_cb(registers_count, name));
}

// (RW)ByteAddressBuffer in shader.
inline ResPtr<Sbuffer> create_ua_sr_byte_address(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_sr_byte_address(size_in_dwords, name, buffer_init));
}
// (RW)StructuredBuffer in shader.
inline ResPtr<Sbuffer> create_ua_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_sr_structured(structure_size, elements_count, name, buffer_init));
}

// RWByteAddressBuffer in shader.
inline ResPtr<Sbuffer> create_ua_byte_address(uint32_t size_in_dwords, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_byte_address(size_in_dwords, name));
}
// RWStructuredBuffer in shader.
inline ResPtr<Sbuffer> create_ua_structured(uint32_t structure_size, uint32_t elements_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_structured(structure_size, elements_count, name));
}

// The same as create_ua_byte_address but its content can be read on CPU
inline ResPtr<Sbuffer> create_ua_byte_address_readback(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_byte_address_readback(size_in_dwords, name, buffer_init));
}
// The same as create_ua_structured but its content can be read on CPU
inline ResPtr<Sbuffer> create_ua_structured_readback(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_structured_readback(structure_size, elements_count, name, buffer_init));
}

// Indirect buffer filled on GPU
inline ResPtr<Sbuffer> create_ua_indirect(Indirect indirect_type, uint32_t records_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_indirect(indirect_type, records_count, name));
}
// Indirect buffer filled on CPU
inline ResPtr<Sbuffer> create_indirect(Indirect indirect_type, uint32_t records_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_indirect(indirect_type, records_count, name));
}

// A buffer for data transfer on GPU
inline ResPtr<Sbuffer> create_staging(uint32_t size_in_bytes, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_staging(size_in_bytes, name));
}

inline ResPtr<Sbuffer> create_persistent_sr_tbuf(uint32_t elements_count, uint32_t format, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_persistent_sr_tbuf(elements_count, format, name, buffer_init));
}

inline ResPtr<Sbuffer> create_persistent_sr_byte_address(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_persistent_sr_byte_address(size_in_dwords, name, buffer_init));
}

inline ResPtr<Sbuffer> create_persistent_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(
    d3d::buffers::create_persistent_sr_structured(structure_size, elements_count, name, buffer_init));
}

inline ResPtr<Sbuffer> create_one_frame_sr_tbuf(uint32_t elements_count, uint32_t format, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_sr_tbuf(elements_count, format, name));
}

inline ResPtr<Sbuffer> create_one_frame_sr_byte_address(uint32_t size_in_dwords, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_sr_byte_address(size_in_dwords, name));
}

inline ResPtr<Sbuffer> create_one_frame_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_sr_structured(structure_size, elements_count, name));
}

inline ResPtr<Sbuffer> create_raytrace_scratch_buffer(uint32_t size_in_bytes, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_raytrace_scratch_buffer(size_in_bytes, name));
}

} // namespace buffers

static inline SharedTex get_tex_gameres(const char *resname) { return SharedTex(::get_tex_gameres(resname, false)); }
static inline SharedTexHolder get_tex_gameres(const char *resname, const char *varname)
{
  return SharedTexHolder(dag::get_tex_gameres(resname), varname);
}
static inline SharedTex add_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm)
{
  return SharedTex(::add_managed_array_texture(name, tex_slice_nm));
}
static inline SharedTexHolder add_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm,
  const char *varname)
{
  return SharedTexHolder(dag::add_managed_array_texture(name, tex_slice_nm), varname);
}
static inline TexPtr place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::place_texture_in_resource_heap(heap, desc, offset, alloc_info, name));
}

static inline BufPtr place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::place_buffer_in_resource_heap(heap, desc, offset, alloc_info, name));
}

static inline ExternalTex get_backbuffer() { return ExternalTex(resptr_detail::ResPtrFactory(d3d::get_backbuffer_tex())); }
static inline ExternalTex get_secondary_backbuffer() // Supposed to be used on Xbox only (autoGameDvr=off mode)
{
  return ExternalTex(resptr_detail::ResPtrFactory(d3d::get_secondary_backbuffer_tex()));
}

static inline TexPtr make_texture_raw(Drv3dMakeTextureParams &makeParams)
{
  Texture *wrappedTexture;
  d3d::driver_command(Drv3dCommand::MAKE_TEXTURE, &makeParams, &wrappedTexture);
  return resptr_detail::ResPtrFactory(wrappedTexture);
}

static inline SharedTexHolder set_texture(int var_id, SharedTex tex) { return SharedTexHolder(eastl::move(tex), var_id); }

static inline SharedBufHolder set_buffer(int var_id, SharedBuf buf) { return SharedBufHolder(eastl::move(buf), var_id); }

static inline SharedTexHolder set_texture(int var_id, const SharedTexHolder &tex)
{
  return SharedTexHolder(SharedTex(tex.getTexId()), var_id);
}

static inline SharedBufHolder set_buffer(int var_id, const SharedBufHolder &buf)
{
  return SharedBufHolder(SharedBuf(buf.getBufId()), var_id);
}

} // namespace dag

#define SCOPED_SET_TEXTURE(a, b) auto DAG_CONCAT(scopedSharedTexGuard, __LINE__) = dag::set_texture(a, b)
#define SCOPED_SET_BUFFER(a, b)  auto DAG_CONCAT(scopedSharedBufGuard, __LINE__) = dag::set_buffer(a, b)
