//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "dag_resMgr.h"
#include "dag_texMgr.h"
#include "dag_drv3d_multi.h"
#include "dag_drv3d_buffers.h"
#include <shaders/dag_shaders.h>
#include <gameRes/dag_gameResources.h>
#include <debug/dag_assert.h>
#include <EASTL/algorithm.h>
#include <EASTL/type_traits.h>

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

/**
  * @brief  Auxiliary class that provides either functions for management of resources 
  *         of generic \ref D3dResource types or placeholders instead, if a specific resource type is required.
  * 
  * @tparam Restype Type of the managed resource. It must be a subclass of \ref D3dResource class.
  */
template <typename ResType>
struct Helper
{

  /**
   * @brief Resource handle (ID) type.
   */
  using resid_t = D3DRESID;

  /**
   * @brief Reserved constant for an invalid resource handle (ID) value.
   */
  static constexpr resid_t BAD_ID = D3DRESID(D3DRESID::INVALID_ID);

  /**
   * @brief Grants the manager ownerwhip of the resource.
   *
   * @param [in] name Name to register the resource under.
   * @param [in] res  A pointer to the resource to register.
   * @return          A handle to the registered resource.
   *
   * @note Use \ref release to delete references and finally destroy the resource.
   * 
   * The registered resource becomes owned by the resource manager, and no factory is assigned to it.
   * The manager maintains a reference count for the resource with the initial value 1.
   * When the counter reaches 0, the resource is destroyed and its name is also unregistered.
   */
  static resid_t registerRes(const char *name, ResType *res) { return register_managed_res(name, res); }

  /**
   * @brief Returns a pointer to the managed resource.
   *
   * @param [in] resId  A handle to the resource to acquire.
   * @return            A pointer to the resource or NULL on error (wrong ID).
   *
   * @note This function usage should be coupled with \ref release.
   * 
   * The function supports reference count, i.e. increments the resource reference count, or
   * if the resource is not referenced yet, creates it. If a factory fails to create the resource,
   * missing texture handling may be applied.
   */
  static ResType *acquire(resid_t resId) { return acquire_managed_res(resId); }

  /**
   * @brief Releases the resource with verification.
   *
   * @param [in, out] resId A handle to the resource to release. It is reset on release.
   * @param [in]      res   A pointer to the resource to check the released resource against.
   *
   * @note This function usage should be coupled with \ref acquire.
   *
   * Calling this function effectively decrements resource reference counter and,
   * if it reaches 0, the resource is genuinely released.
   * In adition to releasing the resource the function checks the released resource against \b check_res.
   * An error is thrown on mismatch.
   */
  static void release(resid_t &resId, ResType *res) { return release_managed_res_verified(resId, res); }

  /**
   * @brief Checks whether a creation/release factory is set for the resource.
   *
   * @param [in] resId  A handle to the resource to check.
   * @return            \c true if a factory is set, \c false otherwise.
   * 
   * @note  This is a no-op for buffers (always returns false). Texture resource specialization has to provide an implementation.
   */
  static bool isManagedFactorySet(resid_t) { return false; }

  /**
   * @brief Retrieves the reference count of the managed resource.
   *
   * @param [in] resId  A handle to the resource.
   * @return            Resource reference count.
   */
  static int getManagedRefCount(resid_t resId) { return get_managed_res_refcount(resId); }

  /**
   * @brief Retrieves the managed resource name.
   *
   * @param [in] res    A pointer to the resource.
   * @return            A string storing the name of the resource or NULL, if on failure.
   * 
   * @note This is a placeholder. Concrete resource type specializations have to provide implementations.
   */
  static const char *getName(ResType *) { return ""; }

  /**
   * @brief Retrieves the managed resource name.
   *
   * @param [in] resId  A handle to the resource.
   * @return            A string storing the name of the resource or NULL, if \b resId is invalid.
   */
  static const char *getNameById(resid_t resId) { return get_managed_res_name(resId); }

  /**
   * @brief Binds the resource to the shader variable.
   * 
   * @param [in] varId  ID of the variable to set.
   * @param [in] resId  A handle to the shader resource to assign.
   * @return            \c true on success, \c false otherwise.
   * 
   * @note This is a placeholder. Concrete resource type specializations have to provide implementations.
   */
  static bool setVar(int, resid_t) { return false; }

  /**
   * @brief Checks whether the resource can be bound to a shader variable.
   * 
   * @param [in] res    A pointer to the resource to check.
   * @return            \c true if binding is possible, \c false otherwise.
   * 
   * @note This is a placeholder. Concrete resource type specializations have to provide implementations.
   */
  static bool can_bind(ResType *) { return false; }
};

/**
 * @brief A \ref Helper class specialization for shader buffer resource type.
 */
template <>
struct Helper<Sbuffer> : public Helper<D3dResource>
{
  /**
   * @brief Returns a pointer to the managed shader buffer.
   *
   * @param [in] resId  A handle to the buffer resource to acquire.
   * @return            A pointer to the buffer or NULL on error (i.e. on wrong ID or on resource not being a shader buffer).
   *
   * @note This function usage should be coupled with \ref release_managed_buf or \ref release_managed_buf_verified.
   * 
   * The function supports reference counting, i.e. increments the buffer reference count, or
   * if the buffer is not referenced yet, creates it.
   */
  static Sbuffer *acquire(resid_t resId) { return acquire_managed_buf(resId); }

  /**
   * @brief Releases the shader buffer with verification.
   *
   * @param [in, out] resId A handle to the buffer to release. It is reset on release.
   * @param [in, out] res   A pointer to the buffer to check the released buffer against. It is set to NULL.
   *
   * @note This function usage should be coupled with \ref acquire_managed_buf.
   *
   * Calling this function effectively decrements resource reference counter and,
   * if it reaches 0, the buffer  resource is genuinely released.
   * In adition to releasing the buffer the function checks the released buffer resource against \b buf.
   * An error is thrown on mismatch.
   */
  static void release(resid_t &resId, Sbuffer *res) { return release_managed_buf_verified(resId, res); }

  /**
   * @brief Retrieves the managed buffer name.
   *
   * @param [in] res    A pointer to the buffer.
   * @return            A string storing the name of the buffer or NULL, if on failure.
   */
  static const char *getName(Sbuffer *res) { return res ? res->getBufName() : nullptr; }

    /**
   * @brief Binds the buffer to the shader variable.
   *
   * @param [in] varId  ID of the variable to set.
   * @param [in] resId  A handle to the shader buffer to assign.
   * @return            \c true on success, \c false otherwise.
   */
  static bool setVar(int varId, resid_t resId) { return ShaderGlobal::set_buffer(varId, resId); }

  /**
   * @brief Mask that is used to check whether buffer creation flags allow resource binding.
   */
  static constexpr uint32_t valid_use_mask = SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED | SBCF_BIND_CONSTANT;

  /**
   * @brief Checks whether the buffer can be bound to a shader variable.
   *
   * @param [in] res    A pointer to the buffer to check.
   * @return            \c true if binding is possible, \c false otherwise.
   */
  static bool can_bind(Sbuffer *res) { return !res || (res->getFlags() & valid_use_mask); }
};

/**
 * @brief A \ref Helper class specialization for texture resource type.
 */
template <>
struct Helper<BaseTexture>
{
  /**
   * @brief Texture handle (ID) type.
   */
  using resid_t = TEXTUREID;

  /**
   * @brief Reserved constant for an invalid texture handle (ID) value.
   */
  static constexpr resid_t BAD_ID = D3DRESID(D3DRESID::INVALID_ID);

  /**
   * @brief Grants the manager ownerwhip of the texture.
   *
   * @param [in] name Name to register the texture under.
   * @param [in] res  A pointer to the texture to register.
   * @return          A handle to the registered texture.
   *
   * The registered texture becomes owned by the resource manager, and no factory is assigned to it.
   * The manager maintains a reference count for the texture with the initial value 1.
   * When the counter reaches 0, the texture is destroyed and its name is also unregistered.
   *
   * @note Use \ref release to delete references and finally destroy the texture.
   */
  static resid_t registerRes(const char *name, BaseTexture *res) { return register_managed_tex(name, res); }

  /**
   * @brief Returns a pointer to the managed texture.
   *
   * @param [in] resId  A handle to the texture to acquire.
   * @return            A pointer to the resource or NULL on error (wrong ID).
   *
   * @note This function usage should be coupled with \ref release.
   * 
   * The function supports reference count, i.e. increments the texture reference count, or
   * if the texture is not referenced yet, creates it. If a factory fails to create the texture,
   * missing texture handling may be applied.
   */
  static BaseTexture *acquire(resid_t resId) { return acquire_managed_tex(resId); }

  /**
   * @brief Releases the texture with verification.
   *
   * @param [in, out] resId A handle to the texture to release. It is reset on release.
   * @param [in]      res   A pointer to the texture to check the released texture against.
   *
   * @note This function usage should be coupled with \ref acquire.
   *
   * Calling this function effectively decrements texture reference counter and,
   * if it reaches 0, the texture is genuinely released.
   * In adition to releasing the texture the function checks the released texture against \b check_res.
   * An error is thrown on mismatch.
   */
  static void release(resid_t &resId, BaseTexture *res) { return release_managed_tex_verified(resId, res); }

  /**
   * @brief Checks whether a creation/release factory is set for the texture.
   *
   * @param [in] resId  A handle to the texture to check.
   * @return            \c true if a factory is set, \c false otherwise.
   */
  static bool isManagedFactorySet(resid_t resId) { return is_managed_tex_factory_set(resId); }

  /**
   * @brief Retrieves the reference count of the managed texture.
   *
   * @param [in] resId  A handle to the texture.
   * @return            Texture reference count.
   */
  static int getManagedRefCount(resid_t resId) { return get_managed_texture_refcount(resId); }

  /**
   * @brief Retrieves the managed texture name.
   *
   * @param [in] res    A pointer to the texture.
   * @return            A string storing the name of the texture or NULL, if on failure.
   */
  static const char *getName(BaseTexture *res) { return res ? res->getTexName() : nullptr; }

  /**
   * @brief Retrieves the managed texture name.
   *
   * @param [in] resId  A handle to the texture (texture ID).
   * @return            A string storing the name of the texture or NULL, if \b resId is invalid.
   */
  static const char *getNameById(resid_t resId) { return get_managed_texture_name(resId); }

  /**
   * @brief Binds the texture to the shader variable.
   *
   * @param [in] varId  ID of the variable to set.
   * @param [in] resId  A handle to the texture to assign.
   * @return            \c true on success, \c false otherwise.
   */
  static bool setVar(int varId, resid_t resId) { return ShaderGlobal::set_texture(varId, resId); }

  /**
   * @brief Checks whether the texture can be bound to a shader variable.
   *
   * @param [in] res    A pointer to the texture to check.
   * @return            \c true if binding is possible, \c false otherwise.
   * 
   * @note Any texture can be bound, so the function always returns \c true.
   */
  static bool can_bind(BaseTexture *) { return true; }
};

/**
 * @brief Dummy class to use as a template parameter for a particular \ref Helper<D3dResource> specialization.
 */
struct ExternalTexWrapper
{};

/**
 * @brief   This specialization implements functionality similar to \ref Helper<BaseTexture>,
 *          but \ref release member function does not actually release the texture, allowing to deal with
 *          resource deallocation externally.
 */
template <>
struct Helper<ExternalTexWrapper> : Helper<BaseTexture>
{
  /**
  * @brief  Mimics resource deallocation behaviour, 
  *         but does not really release the texture allowing external resource deallocation.
  * 
  * @param [in] res_id  ID of the texture resource to release.
  * @param [in] tex     A pointer to the texture to release.
  * @return             \c true on success, \c false otherwise.
  */
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

/**
 * @brief Checks if the resource is a volume texture.
 * 
 * @param [in] res  A pointer to the resource to check.
 * @return          \c true if \b res is a valid pointer to a volume texture, \c false otherwise.
 */
static inline bool isVoltex(D3dResource *res) { return !res || res->restype() == RES3D_VOLTEX; }

/**
 * @brief Checks if the resource is a 2D texture array or a cube texture array.
 * 
 * @param [in] res  A pointer to the resource to check.
 * @return          \c true if \b res is a valid pointer to a 2D texture array or a cube texture array, \c false otherwise.
 */
static inline bool isArrtex(D3dResource *res) { return !res || res->restype() == RES3D_ARRTEX || res->restype() == RES3D_CUBEARRTEX; }

/**
 * @brief Checks if the resource is a cube texture or a cube texture array.
 * 
 * @param [in] res  A pointer to the resource to check.
 * @return          \c true if \b res is a valid pointer to a cube texture or a cube texture array, \c false otherwise.
 */
static inline bool isCubetex(D3dResource *res)
{
  return !res || res->restype() == RES3D_CUBETEX || res->restype() == RES3D_CUBEARRTEX;
}

/**
 * @brief Checks if the resource is a 2D texture.
 * 
 * @param [in] res  A pointer to the resource to check.
 * @return          \c true if b\ res is a valid pointer to a 2D texture, \c false otherwise.
 */
static inline bool isTex2d(D3dResource *res) { return !res || res->restype() == RES3D_TEX; }

/**
 * @brief Checks if the resource is a shader buffer.
 * 
 * @param [in] res  A pointer to the resource to check.
 * @return          \c true if b\ res is a valid pointer to a shader buffer, \c false otherwise.
 */
static inline bool isBuf(D3dResource *res) { return !res || res->restype() == RES3D_SBUF; }

/**
 * @brief Class that provides static member functions to extract resources managed by smart pointers.
 */
struct PrivateDataGetter
{
  /**
   * @brief Retrieves a pointer to the resource managed by the smart pointer.
   * 
   * @tparam        T       Type of
   * 
   * @param [in]    that    A pointer
   * @return                A pointer to the managed resource.
   */
  template <typename T>
  static typename T::derived_t::resource_t *getResource(const T *that)
  {
    return static_cast<const typename T::derived_t *>(that)->mResource;
  }

  /**
   * @brief Retrieves a handle (id) to the resource managed by the smart pointer.
   *
   * @tparam        T       Type of
   *
   * @param [in]    that    A pointer
   * @return                A handle to the managed resource.
   */
  template <typename T>
  static typename T::derived_t::resid_t getResId(const T *that)
  {
    return static_cast<const typename T::derived_t *>(that)->mResId;
  }
};

/**
 * @brief Class that provides functions for getting the texture resource managed by a smart pointer.
 * 
 * @tparam DerivedType Type of the smart pointer class. It must be a subclass of \ref TextureGetter.
 */
template <typename DerivedType>
class TextureGetter
{
public:

  /**
   * @brief Alias for the smart pointer type.
   */
  using derived_t = DerivedType;

  /**
   * @brief Retrieves a pointer to the managed texture base object.
   *
   * @return A pointer to the managed texture base object.
   */
  BaseTexture *getBaseTex() const { return PrivateDataGetter::getResource(this); }

  /**
   * @brief Retrieves a pointer to the managed 2D texture.
   *
   * @return A pointer to the managed 2D texture or NULL if the managed resource is not a 2D texture.
   */
  Texture *getTex2D() const
  {
    return isTex2d(PrivateDataGetter::getResource(this)) ? (Texture *)PrivateDataGetter::getResource(this) : nullptr;
  }

  /**
   * @brief Retrieves a pointer to the managed texture array.
   *
   * @return A pointer to the managed texture array or NULL if the managed resource is not a 2D texture array or a cube texture array.
   */
  ArrayTexture *getArrayTex() const
  {
    return isArrtex(PrivateDataGetter::getResource(this)) ? (ArrayTexture *)PrivateDataGetter::getResource(this) : nullptr;
  }

  /**
   * @brief Retrieves a pointer to the managed cube texture.
   *
   * @return A pointer to the managed cube texture (or cube texture array) or NULL if the managed resource is none of those types.
   */
  CubeTexture *getCubeTex() const
  {
    return isCubetex(PrivateDataGetter::getResource(this)) ? (CubeTexture *)PrivateDataGetter::getResource(this) : nullptr;
  }

  /**
   * @brief Retrieves a pointer to the managed volume texture.
   *
   * @return A pointer to the managed volume texture or NULL if the managed resource is not a volume texture.
   */
  VolTexture *getVolTex() const
  {
    return isVoltex(PrivateDataGetter::getResource(this)) ? (VolTexture *)PrivateDataGetter::getResource(this) : nullptr;
  }

  /**
   * @brief Retrieves a handle to the managed texture.
   *
   * @return A handle to the managed texture.
   */
  TEXTUREID getTexId() const { return PrivateDataGetter::getResId(this); }
};

/**
 * @brief Class that provides functions for getting the buffer resource managed by a smart pointer.
 *
 * @tparam DerivedType Type of the smart pointer class. It must be a subclass of \ref BufferGetter.
 */
template <typename DerivedType>
class BufferGetter
{
public:

  /**
   * @brief Alias for the smart pointer type.
   */
  using derived_t = DerivedType;

  /**
   * @brief Retrieves a pointer to the managed buffer.
   *
   * @note The manahed resource must be a shader buffer, otherwise the function assert-fails.
   * 
   * @return A pointer to the managed buffer.
   */
  Sbuffer *getBuf() const
  {
    G_ASSERT(isBuf(PrivateDataGetter::getResource(this)));
    return (Sbuffer *)PrivateDataGetter::getResource(this);
  }

  /**
   * @brief Retrieves a handle to the managed buffer.
   *
   * @return A handle to the managed buffer.
   */
  D3DRESID getBufId() const { return PrivateDataGetter::getResId(this); }
};

/**
 * @brief An opaque type used to obtain a pointer or a handle to a resource (buffer or texture).
 */
template <typename Type, typename ResType>
using ResourceGetter = eastl::conditional_t<eastl::is_same_v<ResType, Sbuffer>, BufferGetter<Type>, TextureGetter<Type>>;

/**
 * @brief Base class for all (unconditionally) move-assignable resource smart pointers.
 */
class MoveAssignable
{
protected:

  /**
   * @brief Checks if move assignment is valid for objects of a class.
   * 
   * @tparam Type Type of objects to check for move assignment validity. It must be a subclass of \ref MoveAssignable.
   * 
   * @param [in] left, right    Operands of assignment operation.
   * @return                    \c true.
   * 
   * This function is a mean of metaprogramming (compare with \ref WeakMoveAssignable::isValidForMoveAssignment).
   */
  template <typename Type>
  static bool isValidForMoveAssignment(const Type &, const Type &)
  {
    return true;
  }
};

/**
 * @brief Base class for all weak move-assignable resource smart pointers.
 * 
 * Weak move-assignable smart pointers allow move assignment only between pointers that point to different resources.  
 */
class WeakMoveAssignable
{
protected:

  /**
   * @brief Checks if move assignment is valid for objects of a class.
   *
   * @tparam Type Type of objects to check for move assignment validity. It must be a subclass of \ref MoveAssignable.
   *
   * @param [in] left, right    Operands of assignment operation.
   * @return                    \c true if move assignment is valid, \c false otherwise. See \ref WeakMoveAssignable for details.
   *
   * This function is a mean of metaprogramming (compare with \ref MoveAssignable::isValidForMoveAssignment).
   */
  template <typename Type>
  static bool isValidForMoveAssignment(const Type &left, const Type &right)
  {
    return !left || (PrivateDataGetter::getResId(&left) != PrivateDataGetter::getResId(&right));
  }
};

/**
 * @brief Provides all necessary member definitions for enabling moving behavior.
 * 
 * @param   Class  Name of the class to enable move behavior for.
 * 
 * @note    Move assignment will assert failure if objects are not valid for move assignment.
 *          To check this, the \b Class must be a subclass of either \ref MoveAssignable or \ref WeakMoveAssignable.
 * 
 * The macro must be used withing the class definition and provides definitions of
 * default constructor, move constructor, move assignment operator and \c close function.
 */
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

/**
 * @brief Provides a definition for move assignment operator from a resource pointer.
 * 
 * @param [in] Class Name of the class to provide move assignment for.
 * 
 * @note The macro must be used within \b Class definition and only after \ref ENABLE_MOVING_BEHAVIOR has been used.
 */
#define ASSIGNMENT_FROM_RESPTR(Class)       \
  Class &operator=(ResPtr<ResType> &&res)   \
  {                                         \
    close();                                \
    return *this = Class(eastl::move(res)); \
  }

/**
 * @brief Resource smart pointer class.
 * 
 * @tparam ResType Type of the resource.
 * 
 * The managed resource is guaranteed to be destroyed upon smart pointer destruction.
 * An instance of this class can only be constructed via \ref ResPtrFactory function or move constructor.
 */
template <typename ResType>
class ResPtr : public MoveAssignable
{

  /**
   * @brief A pointer to the resource.
   */
  ResType *mResource = nullptr;

  /**
   * @brief Swaps resources between \ref ResPtr objects. 
   * 
   * @param [in] other A pointer to swap with.
   */
  void swap(ResPtr &other) { eastl::swap(mResource, other.mResource); }

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] res    A pointer to the resource to manage.
   */
  ResPtr(ResType *res) : mResource(res) {}

  template <typename Type>
  friend ResPtr<Type> ResPtrFactory(Type *res);

public:
  ENABLE_MOVING_BEHAVIOR(ResPtr);

  /**
   * @brief Destructor.
   * 
   * If the object manages a resource, the destructor destroys it.
   */
  ~ResPtr()
  {
    if (mResource)
      mResource->destroy();
  }

  /**
   * @brief Unbinds the smart pointer from currently managed resource.
   * 
   * @return A pointer to the released resource.
   * 
   * As a result, when \ref ~ResPtr is called, the released resource does not get destroyed.
   */
  ResType *release() &&
  {
    ResType *resource = mResource;
    mResource = nullptr;
    return resource;
  }

  /**
   * @brief Retrieves a pointer to the managed resource.
   * 
   * @return A pointer to the managed resource or NULL if the resource was released.
   */
  ResType *get() const & { return mResource; }

  /**
   * @brief Allows access to the managed resource members;
   * 
   * @return A pointer to the managed resource.
   */
  ResType *operator->() const & { return mResource; }

  /**
   * @brief Checks whether the smart pointer really manages a resource.
   * 
   * @return \c true if the smart pointer really manages a resource, \c false otherwise (i.e. if the resource was released).
   */
  explicit operator bool() const & { return mResource; }
};

/**
 * @brief Constructs \ref ResPtr<ResType> instance (a smart pointer).
 * 
 * @tparam ResType Type of the resource managed by the smart pointer.
 * 
 * @param [in] res  A pointer to the resource to manage.
 * @return          Constructed smart pointer.
 */
template <typename ResType>
static inline ResPtr<ResType> ResPtrFactory(ResType *res)
{
  return {res};
}

/**
 * @brief Class that represents a resource that is to be registered in the resource manager.
 * 
 * @tparam ResType Type of the resource that is being managed. This is either buffer or texture.
 * 
 * @note    Neither does this class own the resource, nor does it register it in the manager. 
 *          This class is to be used for polymorphism (overloads/ metaprogramming).
 */
template <typename ResType>
class ManagedRes : public ResourceGetter<ManagedRes<ResType>, ResType>
{
public:

  /**
   * @brief Resource type alias.
   */
  using resource_t = ResType;

  /**
   * @brief Resource handle (id) type alias.
   */
  using resid_t = typename Helper<ResType>::resid_t;

protected:
  friend PrivateDataGetter;

  /**
   * @brief A handle to the resource.
   */
  resid_t mResId = Helper<ResType>::BAD_ID;

  /**
   * @brief A pointer to the resource.
   */
  ResType *mResource = nullptr;

  /**
   * @brief Swaps resource handles and pointers between to \ref ManagedRes objects.
   */
  void swap(ManagedRes &other)
  {
    eastl::swap(mResId, other.mResId);
    eastl::swap(mResource, other.mResource);
  }

  /**
   * @brief Default constructor.
   */
  ManagedRes() = default;

public:
  /**
   * @brief Provides member access to the resource.
   */
  ResType *operator->() const { return mResource; }

  /**
   * @Checks the resource handle and pointer for validity.
   * 
   * @return \c true if \ref mResource and \ref mResId are valid, \c false otherwise.
   */
  explicit operator bool() const { return mResource && mResId != Helper<ResType>::BAD_ID; }
};

/**
 * @brief   Smart pointer class that creates and/or registers the resource as well as releases it upon destruction. 
 *          The resource is exclusively (uniquely) owned by the object.
 * 
 * @tparam ManagedResType   Texture/ buffer specialization or a subclass of \ref ManagedRes.
 * @tparam Deleter          Type that is used to release the resource. It has to provide static method \c void release(ManagedRes::resid_t &resId, ManagedRes::resource_t *res).
 */
template <typename ManagedResType, typename Deleter = Helper<typename ManagedResType::resource_t>>
class UniqueRes : public ManagedResType, public WeakMoveAssignable
{
public:

  /**
   * @brief Alias for the class providing \c isValidForMoveAssignment function,
   *        that is used to check whether smart pointer allows its resource move assignment.
   *
   * \ref UniqueRes is weakly move-assignable.
   */
  using move_assignable_t = WeakMoveAssignable;

  /**
   * @brief Alias for \ref ManagedResType.
   */
  using BaseType = ManagedResType;

  /**
   * @brief Alias for the owned resource type.
   */
  using ResType = typename BaseType::resource_t;

  ENABLE_MOVING_BEHAVIOR(UniqueRes);
  ASSIGNMENT_FROM_RESPTR(UniqueRes);

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] res            A pointer to the resource to own,
   * @param [in] managed_name   Name to register the resource under. Is NULL is passed, the name is extracted from \b res.
   * 
   * Registers the resource in the manager. 
   * The resource is moved from \b res and becomes owned by the smart pointer. 
   */
  UniqueRes(ResPtr<ResType> &&res, const char *managed_name = nullptr)
  {
    if (!managed_name)
      managed_name = Helper<ResType>::getName(res.get());

    this->mResId = Helper<ResType>::registerRes(managed_name, res.get());
    this->mResource = eastl::move(res).release();
  }

  /**
   * @brief Destructor.
   * 
   * Calls \ref release method.
   */
  ~UniqueRes() { release(this->mResId, this->mResource); }

  /**
   * @brief Releases the resource.
   * 
   * @param [in] res_id A handle to the resource to release.
   * @param [in] res    A pointer to the resource to release.
   * 
   * This unbinds the resource from all shader variables and calls \c release of \ref Deleter. 
   */
  static void release(typename Helper<ResType>::resid_t &res_id, ResType *res)
  {
    ShaderGlobal::reset_from_vars(res_id);
    G_ASSERTF(res_id == Helper<ResType>::BAD_ID || Helper<ResType>::getManagedRefCount(res_id) == 1,
      "resource_name = '%s' res_id=0x%x refCount=%d", res ? res->getResName() : "", res_id,
      Helper<ResType>::getManagedRefCount(res_id));
    Deleter::release(res_id, res);
  }
};

/**
 * @brief   Smart pointer class that creates and/or registers the resource upon initialization as well
 *          as releases it upon destruction. The resource ownership may be shared among objects of class.
 *
 * @tparam ManagedResType   Texture/ buffer specialization or a subclass of \ref ManagedRes.
 * @tparam Deleter          Type that is used to release the resource. It has to provide static method \c void release(ManagedRes::resid_t &resId, ManagedRes::resource_t *res).
 */
template <typename ManagedResType, typename Deleter = Helper<typename ManagedResType::resource_t>>
class SharedRes : public ManagedResType, public MoveAssignable
{
public:

  /**
   * @brief Alias for the class providing \c isValidForMoveAssignment function, 
   *        that is used to check whether smart pointer allows its resource move assignment.
   *
   * \ref SharedRes is unconditionally move-assignable.
   */
  using move_assignable_t = MoveAssignable;

  /**
   * @brief Alias for \ref ManagedResType.
   */
  using BaseType = ManagedResType;

  /**
   * @brief Alias for the owned resource type.
   */
  using ResType = typename BaseType::resource_t;

  ENABLE_MOVING_BEHAVIOR(SharedRes);

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] res_id A handle to the resource to own.
   * 
   * The constructor calls \ref acquire_managed_res function in order to acquire the resource.
   */
  explicit SharedRes(typename Helper<ResType>::resid_t res_id)
  {
    this->mResId = res_id;
    this->mResource = Helper<ResType>::acquire(res_id);
  }

  /**
   * @brief Initialization constructor.
   *
   * @param [in] res A pointer to the resource to own.
   *
   * The constructor calls \ref register_managed_res in order to register \b res.
   */
  explicit SharedRes(ResPtr<ResType> &&res)
  {
    this->mResource = eastl::move(res).release();
    this->mResId = Helper<ResType>::registerRes(Helper<ResType>::getName(this->mResource), this->mResource);
  }

  /**
   * @brief Copy constructor.
   * 
   * @param [in] other An object to share ownership over a common resource with.
   */
  SharedRes(const SharedRes &other) : SharedRes(other.mResId) {}

  /**
   * @brief Copy assignment operator.
   * 
   * @param [in] other Object to share ownership over a common resource with.
   */
  SharedRes &operator=(const SharedRes &other)
  {
    SharedRes(other).swap(*this);
    return *this;
  }

  /**
   * @brief Destructor.
   * 
   * Calls \ref release member function.
   */
  ~SharedRes() { release(this->mResId, this->mResource); }

  /**
   * @brief Evicts the resource if necessary.
   * 
   * @param [in] res_id A handle to the resource to release.
   * @param [in] res    A pointer to the resource to release.
   * 
   * @note  This function calls \ref release function in \ref Deleter. 
            It is responsible for maintaining the resource reference count.
   */
  static void release(typename Helper<ResType>::resid_t &res_id, ResType *res)
  {
    Deleter::release(res_id, res);
    if (Helper<ResType>::getManagedRefCount(res_id) >= 0)
      evict_managed_tex_id(res_id);
  }
};

/**
 * @brief Class that provides non-owning access to a managed resource.
 * 
 * @tparam ManagedResType Smart pointer class that data will copied from. It has to be a texture/ buffer specialization or a subclass of \ref ManagedRes.
 * 
 * This class does not create, register or release the resource, nor it maintains its reference count. 
 * It is possible to use this class objects as parameters for \ref PrivateDataGetter member functions.
 * For those features use owning pointers like \ref UniqueRes and \ref SharedRes.
 */
template <typename ManagedResType>
class ManagedResView : public ManagedResType
{
public:

  /**
   * @brief Alias for ManagedResType.
   */
  using BaseType = ManagedResType;

  /**
   * @brief Alias for the resource type.
   */
  using ResType = typename BaseType::resource_t;

  /**
   * @brief Default constructor.
   */
  ManagedResView() = default;

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] Smart pointer to the resource.
   * 
   * Copies from the smart pointer.
   */
  ManagedResView(const ManagedResType &res)
  {
    this->mResId = PrivateDataGetter::getResId(&res);
    this->mResource = PrivateDataGetter::getResource(&res);
  }

  /**
   * @brief Initialization constructor (deleted).
   * 
   * @param [in] Smart pointer to the resource.
   * 
   * Moves from the smart pointer.
   */
  ManagedResView(ManagedResType &&res) = delete;
};

#if DAGOR_DBGLEVEL > 1
// This is a trick to create a global variable in the header.
// Despite the fact that this header will be included to different cpp,
// the linker will leave only one copy of this variable

/**
 * @brief Stores shader variable ids that are already bound to some resources.
 */
template <typename T = void>
eastl::unordered_set<int> gUsedShaderVarIds;

/**
 * @brief Registers the value as a new shader variable id.
 * 
 * @brief [in] new_id Value to register.
 * 
 * An error is thrown if \b new_id is already bound to a shader variable.
 */
static void acquireShaderVarId(int new_id)
{
  if (new_id != VariableMap::BAD_ID && !gUsedShaderVarIds<>.emplace(new_id).second)
    DAG_FATAL("You are trying to bind resource to a shaderVar with id = %d"
              " to which another resource is already bound, this can lead to unpredictable bugs",
      new_id);
}

/**
 * @brief Unregisters the shader id value allowing it to be reused.
 *
 * @brief [in] new_id Value to unregister.
 */
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

/**
 * Resource smart pointer class that provides means to bind the managed resource to a shader variable.
 * 
 * @tparam ManagedResType Subclass\specialization of \ref ManagedRes<ResType>, for example \c UniqueRes<SBuffer> or \c SharedRes<BaseTexture>.
 */
template <typename ManagedResType>
class ManagedResHolder : public ManagedResType
{
  /**
   * @brief Alias for the resource type.
   */
  using ResType = typename ManagedResType::resource_t;

protected:

  /**
   * @brief Id of the shader variable that the resource is bound to.
   */
  int mShaderVarId = -1;

  /**
   * @brief Swaps the managed resources.
   * 
   * @param [in] other A pointer to the resource to swap with.
   */
  void swap(ManagedResHolder &other)
  {
    ManagedResType::swap(other);
    eastl::swap(mShaderVarId, other.mShaderVarId);
  }

  /**
   * @brief Default constructor. 
   */
  ManagedResHolder() = default;

public:

  /**
   * @brief Retrieves the id of the shader variable the managed resource is bound to.
   */
  int getVarId() const { return mShaderVarId; }

  /**
   * @brief Binds the shader variable (pointed at by \ref mShaderVarId) to the managed resource.
   */
  void setVar() const
  {
    if (mShaderVarId != -1)
      Helper<ResType>::setVar(mShaderVarId, this->mResId);
  }

  /**
   * @brief Sets the shader variable id to a specified value.
   * 
   * @param [in] var_id Value to assign.
   */
  void setVarId(int var_id)
  {
    releaseShaderVarId(mShaderVarId);
    acquireShaderVarId(var_id);
    mShaderVarId = var_id;
  }
};

/**
 * @brief Resource smart pointer class that provides means to bind the managed resource to a shader variable and also exhibits moving behavior.
 * 
 * @tparam ManagedResType Subclass\specialization of \ref ManagedRes<ResType>, for example \c UniqueRes<SBuffer> or \c SharedRes<BaseTexture>.
 */
template <typename ManagedResType>
class ConcreteResHolder : public ManagedResHolder<typename ManagedResType::BaseType>, public ManagedResType::move_assignable_t
{

  /**
  * @brief Alias for the resource type.
  */
  using ResType = typename ManagedResType::resource_t;

public:
  ENABLE_MOVING_BEHAVIOR(ConcreteResHolder);
  ASSIGNMENT_FROM_RESPTR(ConcreteResHolder);

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] resource           Resource to own.
   * @param [in] shader_var_name    Name of the shader variable to create the resource binding under. If NULL is passed, the name to use is taken from \b resource.
   * 
   * @note Calling this constructor renders \resource 'empty'.
   */
  ConcreteResHolder(ManagedResType &&resource, const char *shader_var_name = nullptr)
  {
    ManagedResType::BaseType::swap(resource);

    if (!shader_var_name)
      shader_var_name = Helper<ResType>::getNameById(this->mResId);

    this->setVarId(get_shader_variable_id(shader_var_name));
    if (Helper<ResType>::can_bind(this->mResource))
      this->setVar();
  }

  /**
   * @brief Initialization constructor.
   *
   * @param [in] resource   Resource to own.
   * @param [in] var_id     Id of the shader variable to create the resource binding under.
   * 
   * @note Calling this constructor renders \resource 'empty'.
   */
  ConcreteResHolder(ManagedResType &&resource, int var_id)
  {
    ManagedResType::BaseType::swap(resource);

    this->setVarId(var_id);
    if (Helper<ResType>::can_bind(this->mResource))
      this->setVar();
  }

  /**
   * @brief Initialization constructor.
   *
   * @param [in] res                Resource to own.
   * @param [in] shader_var_name    Name of the shader variable to create the resource binding under. If NULL is passed, the name to use is taken from \b resource.
   */
  ConcreteResHolder(ResPtr<ResType> &&res, const char *shader_var_name = nullptr) :
    ConcreteResHolder(ManagedResType(eastl::move(res)), shader_var_name)
  {}

  /**
   * @brief Destructor.
   * 
   * Releases shader variable and evicts the resource if necessary. 
   */
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

/**
 * @brief Alias for the texture resource type managed by the manager.
 */
using ManagedTex = ManagedRes<BaseTexture>;

/**
 * @brief Alias for the texture resource type managed by the manager.
 */
using ManagedBuf = ManagedRes<Sbuffer>;

template <typename ManagedResType>
using UniqueRes = resptr_detail::UniqueRes<ManagedResType>;

/**
 * @brief Alias for the type of a unique pointer to a texture resource managed by the manager.
 */
using UniqueTex = UniqueRes<ManagedTex>;

/**
 * @brief Alias for the type of a unique pointer to a buffer resource managed by the manager.
 */
using UniqueBuf = UniqueRes<ManagedBuf>;

/**
 * @brief Alias for the type of a texture unique pointer with external control over deallocation.
 */
using ExternalTex = resptr_detail::UniqueRes<ManagedTex, resptr_detail::Helper<resptr_detail::ExternalTexWrapper>>;
// implement change_managed_buf for ExternalBuf

template <typename ManagedResType>
using ManagedResView = resptr_detail::ManagedResView<ManagedResType>;

/**
 * @brief Alias for the type of a texture view.
 */
using ManagedTexView = ManagedResView<ManagedTex>;

/**
 * @brief Alias for the type of a buffer view.
 */
using ManagedBufView = ManagedResView<ManagedBuf>;

template <typename ManagedResType>
using SharedRes = resptr_detail::SharedRes<ManagedResType>;

/**
 * @brief Alias for the type of a shared pointer to a texture resource managed by the manager.
 */
using SharedTex = SharedRes<ManagedTex>;

/**
 * @brief Alias for the type of a shared pointer to a buffer resource managed by the manager.
 */
using SharedBuf = SharedRes<ManagedBuf>;

/**
 * @brief Alias for \ref ManagedResHolder type specification for texture resources.
 */
using ManagedTexHolder = resptr_detail::ManagedResHolder<ManagedTex>;

/**
 * @brief Alias for \ref ManagedResHolder type specification for buffer resources.
 */
using ManagedBufHolder = resptr_detail::ManagedResHolder<ManagedTex>; //Perhaps, a misprint. -> using ManagedBufHolder = resptr_detail::ManagedResHolder<ManagedBuf>

/**
 * @brief Alias for \ref ConcreteResHolder type specification for texture unique pointers.
 */
using UniqueTexHolder = resptr_detail::ConcreteResHolder<UniqueTex>;

/**
 * @brief Alias for \ref ConcreteResHolder type specification for buffer unique pointers.
 */
using UniqueBufHolder = resptr_detail::ConcreteResHolder<UniqueBuf>;

/**
 * @brief Alias for \ref ConcreteResHolder type specification for texture shared pointers.
 */
using SharedTexHolder = resptr_detail::ConcreteResHolder<SharedTex>;

/**
 * @brief Alias for \ref ConcreteResHolder type specification for buffer shared pointers.
 */
using SharedBufHolder = resptr_detail::ConcreteResHolder<SharedBuf>;

template <typename ResType>
using ResPtr = resptr_detail::ResPtr<ResType>;

/**
 * @brief Alias for texture smart pointer type.
 */
using TexPtr = ResPtr<BaseTexture>;

/**
 * @brief Alias for shader buffer smart pointer type.
 */
using BufPtr = ResPtr<Sbuffer>;

namespace dag
{

/**
 * @brief Creates a texture object from an image.
 *
 * @param [in] img          A pointer to the image to create the texture from. If \c NULL is passed, the texture will be created empty.
 * @param [in] w            Texture width.
 * @param [in] h            Texture height.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       Number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture.
 * @return                  A pointer to the created texture object or NULL on error.
 *
 * @note If \c NULL is passed for \b img, proper values for \b w and \b h must be specified.
 * @note If either \b w or \h is 0, the corresponding value is taken from \b img (hence, it must not be \c NULL).
 */
static inline ResPtr<BaseTexture> create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_tex(img, w, h, flg, levels, name));
}

/**
 * @brief Creates a cube texture object.
 *
 * @param [in] size         Cube texture edge length.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       Number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture.
 * @return                  A pointer to the created texture object or NULL on error.
 */
static inline ResPtr<CubeTexture> create_cubetex(int size, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_cubetex(size, flg, levels, name));
}

/**
 * @brief Creates a volume texture object.
 *
 * @param [in] w, h, d      Volume texture dimensions.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       Number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture.
 * @return                  A pointer to the created texture object or NULL on error.
 */
static inline ResPtr<VolTexture> create_voltex(int w, int h, int d, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_voltex(w, h, d, flg, levels, name));
}

/**
 * @brief Creates a texture array object.
 *
 * @param [in] w, h         Texture dimensions.
 * @param [in] d            Number of layers in the array.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       Number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture array.
 * @return                  A pointer to the created texture array object or NULL on error.
 */
static inline ResPtr<ArrayTexture> create_array_tex(int w, int h, int d, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_array_tex(w, h, d, flg, levels, name));
}

/**
 * @brief Creates a cube texture array object.
 *
 * @param [in] side         Cube texture edge length.
 * @param [in] d            Number of layers in the array.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       Number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture array.
 * @return                  A pointer to the created texture array object or NULL on error.
 *
 * @note The function actually creates array of <c>d * 6 <\c> simple textures, where each group of 6 layers makes up a cube map.
 */
static inline ResPtr<ArrayTexture> create_cube_array_tex(int side, int d, int flg, int levels, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_cube_array_tex(side, d, flg, levels, name));
}

/**
 * @brief Creates a texture object from a DDSx stream.
 *
 * @param [in] crd          A callback supporting file read interface.
 * @param [in] flg          Texture creation flags.
 * @param [in] quality_id   Texture quality level index (0=high, 1=medium, 2=low, 3=ultralow).
 * @param [in] levels       Number of mipmaps to load. If 0 is passed, the whole set is loaded, given the device support.
 * @param [in] stat_name    Texture debug name.
 * @return                  A pointer to the created texture object or NULL on error.
 */
static inline TexPtr create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels = 0, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::create_ddsx_tex(crd, flg, quality_id, levels, name));
}

/**
 * @brief Creates a simple texture alias from another texture object.
 *
 * @param [in] baseTexture  A pointer to the texture to create an alias for.
 * @param [in] img          A pointer to the image to create the texture from. If \c NULL is passed, the texture will be created empty.
 * @param [in] w, h         Texture dimensions.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       Number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture alias.
 * @return                  A pointer to the created alias or NULL on error.
 *
 * An alias can be considered as a view of or as a cast to its base texture but of a different format.
 * Calling this function has same effects as \ref create_tex, except creating alias does not result in memory allocation,
 * and the GPU memory of the base texture will be used as the backing GPU memory for the alias.
 * For an alias is just a view of the GPU memory area, its lifetime is controlled by the base texture.
 * Hence, no alias texture should be used in the rendering pipeline after its base texture has been deleted.
 *
 * @note    If \c NULL is passed for \b img, proper values for \b w and \b h must be specified.
 * @note    If either \b w or \h is 0, the corresponding value is taken from \b img (hence, it must not be \c NULL).
 * @warning Passing non-NULL for \b img will result in \b baseTexture content overwrite.
 */
static inline TexPtr alias_tex(BaseTexture *base_tex, TexImage32 *img, int w, int h, int flg, int levels, const char *name = nullptr)
{
  return resptr_detail::ResPtrFactory(d3d::alias_tex(base_tex, img, w, h, flg, levels, name));
}

/**
 * @brief Creates an empty vertex buffer and adds it to the buffer list.
 *
 * @param [in] size_bytes   Buffer size.
 * @param [in] flags        Buffer creation flags.
 * @param [in] name         Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
static inline ResPtr<Vbuffer> create_vb(int sz, int f, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_vb(sz, f, name));
}

/**
 * @brief Creates an empty index buffer and adds it to the buffer list.
 *
 * @param [in] size_bytes   Buffer size.
 * @param [in] flags        Buffer creation flags.
 * @param [in] stat_name    Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
static inline ResPtr<Ibuffer> create_ib(int size_bytes, int flags, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::create_ib(size_bytes, flags, name));
}

/**
 * @brief Creates a structured buffer and adds it to the buffer list.
 *
 * @param [in] struct_size  Size of the structure.
 * @param [in] elements     Number of elements in the buffer.
 * @param [in] flags        Buffer creation flags.
 * @param [in] texfmt       Element format (for non-structured buffers used in rendering).
 * @param [in] name         Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
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
  /**
   * @brief Creates a persistent constant buffer.
   *
   * @param registers_count Number of registers in the buffer. Must be not bigger than 4096.
   * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
   * @return                Created persistent constant buffer.
   *
   * @warning This buffer will not be restored after device reset!
   * 
   * Such buffers could be updated from time to time.
   * It is recommended to use \ref cb_array_reg_count and \ref cb_struct_reg_count methods to calculate registers_count.
   */
  return resptr_detail::ResPtrFactory(d3d::buffers::create_persistent_cb(registers_count, name));
}

/**
 * @brief Creates a one frame constant buffer.
 *
 * @param registers_count   Number of registers. Must be not bigger than 4096.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @param buffer_init       Initialization option for the buffer.
 * @return                  A pointer to the created buffer.
 * 
 * Such buffers must be updated every frame (you can skip update if the buffer is not used this frame).
 * Because of that we don't care about its content on device reset.
 * It is recommended to use \ref cb_array_reg_count and \ref cb_struct_reg_count methods to calculate registers_count.
 */
inline ResPtr<Sbuffer> create_one_frame_cb(uint32_t registers_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_cb(registers_count, name));
}

/**
 * @brief Creates a byte address buffer.
 * 
 * @param size_in_dwords    Size of the buffer in dwords.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @param buffer_init       Initialization option for the buffer.
 * @return                  A pointer to the created buffer.
 * 
 * @todo Use registers instead of dwords for size because of alignment.
 * 
 * Such buffers can be used via unordered access view or via shader resource view in
 * shaders. In a shader you can declare a BA buffer using (RW)ByteAddressBuffer.
 * Such buffer is always 16-byte aligned.
 */
inline ResPtr<Sbuffer> create_ua_sr_byte_address(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_sr_byte_address(size_in_dwords, name, buffer_init));
}

/**
 * @brief   Create a structured buffer, which can be used via unordered access view or via shader resource view in shaders.
 *          In a shader you can declare the buffer using <c> (RW)StructuredBuffer<StructureType> </c>. StructureType is a kind of template parameter
 *          here.
 *
 * @param structure_size    Size of the structure of the buffer elements. Usually it is \c sizeof(StructureType).
 * @param elements_count    Number of elements in the buffer.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @param buffer_init       Initialization option for the buffer.
 * @return                  A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_ua_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_sr_structured(structure_size, elements_count, name, buffer_init));
}

/**
 * @brief   Creates a byte address buffer, which can be used via unordered access view in shaders.
 *          In a shader you can declare the buffer using \c RWByteAddressBuffer.
 *          Such a buffer is always 16-byte aligned.
 *
 * @param size_in_dwords    Size of the buffer in dwords.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return                  A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_ua_byte_address(uint32_t size_in_dwords, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_byte_address(size_in_dwords, name));
}

/**
 * @brief   Creates a structured buffer, which can be used via unordered access view in shaders.
 *          In a shader you can declare the buffer using <c> RWStructuredBuffer<StructureType> <c>. StructureType is a kind of template parameter here.
 *
 * @param structure_size    Size of the structure of the buffer elements. Usually it is \c sizeof(StructureType).
 * @param elements_count    Number of elements in the buffer.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return                  A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_ua_structured(uint32_t structure_size, uint32_t elements_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_structured(structure_size, elements_count, name));
}

// The same as create_ua_byte_address but its content can be read on CPU
/**
 * @brief   Creates a byte address buffer, which can be used via unordered access view in shaders and read on CPU.
 *          In a shader you can declare the buffer using \c RWByteAddressBuffer.
 *          Such a buffer is always 16-byte aligned.
 *
 * @param size_in_dwords    Size of the buffer in dwords.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @param buffer_init       Initialization option for the buffer.
 * @return                  A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_ua_byte_address_readback(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_byte_address_readback(size_in_dwords, name, buffer_init));
}

/**
 * @brief   Creates a structured buffer, which can be used via unordered access view in shaders and read on CPU.
 *          In a shader you can declare the buffer using <c> RWStructuredBuffer<StructureType> <c>. StructureType is a kind of template
 *          parameter here.
 *
 * @param structure_size    Size of the structure of buffer elements. Usually it is \c sizeof(StructureType).
 * @param elements_count    Number of elements in the buffer.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return                  A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_ua_structured_readback(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_structured_readback(structure_size, elements_count, name, buffer_init));
}

/**
 * @brief Creates an indirect buffer filled by the GPU.
 *
 * @param indirect_type Type of the indirect commands stored in the buffer.
 * @param records_count Number of indirect records in the buffer.
 * @param name          Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return              A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_ua_indirect(Indirect indirect_type, uint32_t records_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_ua_indirect(indirect_type, records_count, name));
}

/**
 * @brief Creates an indirect buffer filled by the CPU.
 *
 * @param indirect_type Type of the indirect commands stored in the buffer.
 * @param records_count Number of indirect records in the buffer.
 * @param name          Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return              A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_indirect(Indirect indirect_type, uint32_t records_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_indirect(indirect_type, records_count, name));
}

/**
 * @brief Creates a buffer for data transfer from the CPU to the GPU.
 *
 * @param size_in_bytes Size of the buffer in bytes.
 * @param name          Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return              A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_staging(uint32_t size_in_bytes, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_staging(size_in_bytes, name));
}

/**
 * @brief Create a t-buffer which can be used through a shader resource view in shaders.
 *
 * @param elements_count    Number of elements in the buffer.
 * @param format            Format of each element in the buffer. It must be a valid texture format. Not all texture formats are allowed.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @param buffer_init       Initialization option for the buffer.
 * @return                  A pointer to the created buffer.
 * 
 * @warning The buffer type can be used only for the code which will be used for DX10-compatible hardware.
 * 
 * In a shader you can declare the buffer using Buffer<BufferFormat>. BufferFormat is a kind of template parameter here. \n
 * The total size of the buffer is <c> sizeof(format) * elements_count </c>. \n
 * It is a persistent buffer, so you can update it with \c VBLOCK_WRITEONLY flag. Locked part of the buffer content will be overwritten.
 */
inline ResPtr<Sbuffer> create_persistent_sr_tbuf(uint32_t elements_count, uint32_t format, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_persistent_sr_tbuf(elements_count, format, name, buffer_init));
}

/**
 * @brief Create a byte address buffer which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using ByteAddressBuffer.
 *
 * It is a persistent buffer, so you can update it with VBLOCK_WRITEONLY flag. Locked part of the buffer content will be overwritten.
 *
 * @param size_in_dwords    Size of the buffer in dwords.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @param buffer_init       Initialization option for the buffer.
 * @return                  A pointer to the created buffer.
 */
inline ResPtr<Sbuffer> create_persistent_sr_byte_address(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_persistent_sr_byte_address(size_in_dwords, name, buffer_init));
}

/**
 * @brief Create a structured buffer which can be used through a shader resource view in shaders.
 *
 * @param structure_size Size of the structure of the buffer elements. Usually it is \c sizeof(StructureType).
 * @param elements_count Number of elements in the buffer.
 * @param name           Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.* 
 * @param buffer_init    Initialization option for the buffer.
 * @return               A pointer to the created buffer.
 * 
 * It is a persistent buffer, so you can update it with \c VBLOCK_WRITEONLY flag. Locked part of the buffer content will be overwritten. \n
 * In a shader you can declare the buffer using \c StructuredBuffer<StructureType>. \c StructureType is a kind of template parameter here.
 * Declare \c StructureType in *.hlsli file and include it both in C++ and shader code.
 */
inline ResPtr<Sbuffer> create_persistent_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return resptr_detail::ResPtrFactory(
    d3d::buffers::create_persistent_sr_structured(structure_size, elements_count, name, buffer_init));
}

/**
 * @brief Create a t-buffer which can be used through a shader resource view in shaders.
 *
 * @param elements_count    Number of elements in the buffer.
 * @param format            Format of each element in the buffer. It must be a valid texture format. Not all texture formats are allowed.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return                  A pointer to the created buffer.
 * 
 * @warning The buffer type can be used only for the code which will be used for DX10-compatible hardware.
 *
 * In a shader you can declare the buffer using \c Buffer<BufferFormat>. \c BufferFormat is a kind of template parameter here.
 * The total size of the buffer is <c> sizeof(format) * elements_count </c>.
 * To use the buffer, lock it with \c VBLOCK_DISCARD flag (and with \c VBLOCK_NOOVERWRITE during the frame) and fill it on the CPU.
 * Next frame data in the buffer could be invalid, so don't read from it until you fill it with lock again.
 */
inline ResPtr<Sbuffer> create_one_frame_sr_tbuf(uint32_t elements_count, uint32_t format, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_sr_tbuf(elements_count, format, name));
}

/**
 * @brief Create a byte address buffer which can be used through a shader resource view in shaders.
 *
 * @param size_in_dwords    The size of the buffer in dwords.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return                  A pointer to the created buffer.
 * 
 * In a shader you can declare the buffer using \c ByteAddressBuffer.
 * To use the buffer, lock it with \c VBLOCK_DISCARD flag (and with \c VBLOCK_NOOVERWRITE during the frame) and fill it on the CPU.
 * On the next frame data in the buffer could be invalid, so don't read from it until you fill it with lock again.
 */
inline ResPtr<Sbuffer> create_one_frame_sr_byte_address(uint32_t size_in_dwords, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_sr_byte_address(size_in_dwords, name));
}

/**
 * @brief Create a structured buffer which can be used through a shader resource view in shaders.
 *
 * @param structure_size    Size of the structure of the buffer elements. Usually it is a <c> sizeof(StructureType) </c>.
 * @param elements_count    Number of elements in the buffer.
 * @param name              Name of the buffer used for debugging purposes like showing in in statistcs and frame debuggers like PIX.
 * @return                  A pointer to the created buffer.
 * 
 * In a shader you can declare the buffer using <c> StructuredBuffer<StructureType> </c>. /c StructureType is a kind of template parameter here.
 * To use the buffer, lock it with \c VBLOCK_DISCARD flag (and with \c VBLOCK_NOOVERWRITE during the frame) and fill it on CPU.
 * On the next frame data in the buffer could be invalid, so don't read from it until you fill it with lock again.
 * Declare \c StructureType in *.hlsli file and include it both in C++ and shader code.
 */
inline ResPtr<Sbuffer> create_one_frame_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::buffers::create_one_frame_sr_structured(structure_size, elements_count, name));
}

} // namespace buffers

/**
 * @brief Retrieves a pointer to the texture by its name.
 * 
 * @param [in] resname  Name of the texture.
 * @return              A pointer to the resource.
 */
static inline SharedTex get_tex_gameres(const char *resname) { return SharedTex(::get_tex_gameres(resname, false)); }

/**
 * @brief Constructs a texture holder by the texture name.
 * 
 * @param [in] resname  Name of the texture.
 * @param [in] varname  Name of shader variable to bind the texture to.
 * @param               Resulted texture holder.
 */
static inline SharedTexHolder get_tex_gameres(const char *resname, const char *varname)
{
  return SharedTexHolder(dag::get_tex_gameres(resname), varname);
}

/**
 * @brief Adds a texture array to the manager or updates an existing one.
 * 
 * @param [in] name         Texture array name.
 * @param [in] tex_slice_nm A span of names for array slices.
 * @return                  A pointer to the texture array resource.
 */
static inline SharedTex add_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm)
{
  return SharedTex(::add_managed_array_texture(name, tex_slice_nm));
}

/**
 * @brief Adds a texture array to the manager or updates an existing one and binds it to the shader variable.
 *
 * @param [in] name         Texture array name.
 * @param [in] tex_slice_nm A span of names for array slices.
 * @param [in] varname      Name of the shader variable.
 * @return                  A pointer to the texture array resource.
 */
static inline SharedTexHolder add_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm,
  const char *varname)
{
  return SharedTexHolder(dag::add_managed_array_texture(name, tex_slice_nm), varname);
}

/**
 * @brief Creates a texture and places it to the resource heap.
 *
 * @param [in] heap         A pointer to the resource heap to place the texture to.
 * @param [in] desc         Description of the texture.
 * @param [in] offset       Byte offset from the top of the heap at which the texture is placed.
 * @param [in] alloc_info   Texture allocation properties.
 * @param [in] name         Texture debug name.
 * @return                  A pointer to the created texture.
 */
static inline TexPtr place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::place_texture_in_resource_heap(heap, desc, offset, alloc_info, name));
}

/**
 * @brief Creates a buffer and places it to the resource heap.
 *
 * @param [in] heap         A pointer to the resource heap to place the buffer to.
 * @param [in] desc         Description of the buffer.
 * @param [in] offset       Byte offset from the top of the heap at which the buffer is placed.
 * @param [in] alloc_info   Buffer allocation properties.
 * @param [in] name         Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
static inline BufPtr place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return resptr_detail::ResPtrFactory(d3d::place_buffere_in_resource_heap(heap, desc, offset, alloc_info, name));
}

/**
 * @brief Retrieves a pointer to the backbuffer color texture.
 *
 * @return A pointer to the backbuffer color texture.
 *
 * @note Backbuffer is only valid while the GPU is acquired, and can be recreated in between.
 */
static inline ExternalTex get_backbuffer() { return ExternalTex(resptr_detail::ResPtrFactory(d3d::get_backbuffer_tex())); }

/**
 * @brief Retrieves a pointer to the backbuffer depth texture.
 *
 * @return A pointer to the backbuffer depth texture.
 *
 * @note Backbuffer is only valid while the GPU is acquired, and can be recreated in between.
 */
static inline ExternalTex get_backbuffer_depth() { return ExternalTex(resptr_detail::ResPtrFactory(d3d::get_backbuffer_tex_depth())); }

/**
 * @brief Constructs a texture object.
 * 
 * @param [in] makeParams   Texture construction parameters.
 * @return                  Smart pointer to the constructed texture.
 */
static inline TexPtr make_texture_raw(Drv3dMakeTextureParams &makeParams)
{
  Texture *wrappedTexture;
  d3d::driver_command(DRV3D_COMMAND_MAKE_TEXTURE, &makeParams, &wrappedTexture, nullptr);
  return resptr_detail::ResPtrFactory(wrappedTexture);
}

/**
 * @brief Binds the texture to the shader variable.
 *
 * @param [in] var_id   Id of the shader variable to bind the texture to.
 * @param [in] buf      Smart pointer managing the texture.
 * @return              Resulted texture holder.
 */
static inline SharedTexHolder set_texture(int var_id, SharedTex tex) { return SharedTexHolder(eastl::move(tex), var_id); }

/**
 * @brief Binds the shader buffer to the shader variable.
 *
 * @param [in] var_id   Id of the shader variable to bind the buffer to.
 * @param [in] buf      Smart pointer managing the buffer.
 * @return              Resulted buffer holder.
 */
static inline SharedBufHolder set_buffer(int var_id, SharedBuf buf) { return SharedBufHolder(eastl::move(buf), var_id); }

/**
 * @brief Binds the texture to the shader variable.
 *
 * @param [in] var_id   Id of the shader variable to bind the texture to.
 * @param [in] tex      Texture holder managing the buffer.
 * @return              Resulted texture holder.
 */
static inline SharedTexHolder set_texture(int var_id, const SharedTexHolder &tex)
{
  return SharedTexHolder(SharedTex(tex.getTexId()), var_id);
}

/**
 * @brief Binds the shader buffer to the shader variable.
 * 
 * @param [in] var_id   Id of the shader variable to bind the buffer to.
 * @param [in] buf      Buffer holder managing the buffer.
 * @return              Resulted buffer holder.
 */
static inline SharedBufHolder set_buffer(int var_id, const SharedBufHolder &buf)
{
  return SharedBufHolder(SharedBuf(buf.getBufId()), var_id);
}

} // namespace dag

//@cond
#define RESPTR_CAT(a, b)         a##b
#define RESPTR_CAT2(a, b)        RESPTR_CAT(a, b)
//@endcond

/**
 * @brief Binds a texture to a shader variable within a scope. As soon as execution leaves the scope, the texture gets unbound.
 * 
 * @param [in] a Id of the shader variable to bind the texture to.
 * @param [in] b A smart pointer to the texture to bind.
 */
#define SCOPED_SET_TEXTURE(a, b) auto RESPTR_CAT2(scopedSharedTexGuard, __LINE__) = dag::set_texture(a, b)

/**
 * @brief Binds a buffer to a shader variable within a scope. As soon as execution leaves the scope, the buffer gets unbound.
 *
 * @param [in] a Id of the shader variable to bind the buffer to.
 * @param [in] b A smart pointer to the buffer to bind.
 */
#define SCOPED_SET_BUFFER(a, b)  auto RESPTR_CAT2(scopedSharedBufGuard, __LINE__) = dag::set_buffer(a, b)
