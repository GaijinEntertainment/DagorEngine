//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>

#include <render/daFrameGraph/detail/virtualResourceHandleBase.h>

#include <3d/dag_resPtr.h>
#include <render/daFrameGraph/resourceView.h>


namespace dafg
{

namespace detail
{
enum class ResourceRequestPolicy : uint32_t;
}

#ifndef DOXYGEN
// TODO: Replace this hackyness with constraints as soon as C++20 drops
#define REQUIRE(...) template <class Dummy1 = void, class = typename eastl::enable_if<(__VA_ARGS__), Dummy1>::type>
#else
#define REQUIRE(...)
#endif

/**
 * \brief Handle to a virtual resource managed by FG. Does not actually
 * represent a physical GPU or CPU resource until the execution callback
 * gets called. The physical resource will also change between calls to
 * the execution callback, so the dereferenced value of this handle
 * should NEVER be cached anywhere.
 * \details All methods are const & as this is supposed to be called from
 * within the node execution callback.
 * \tparam Res BaseTexture or Sbuffer when \p gpu == true, otherwise arbitrary CPU data type.
 * \tparam gpu True iff this handle represents a GPU resource.
 * \tparam optional True iff this handle can refer to a missing resource.
 */
template <class Res, bool gpu, bool optional>
class VirtualResourceHandle
  /// \cond DETAIL
  : private detail::VirtualResourceHandleBase
/// \endcond
{
  template <class, detail::ResourceRequestPolicy>
  friend class VirtualResourceRequest;

  using Base = detail::VirtualResourceHandleBase;

  using ResNoConst = eastl::remove_cvref_t<Res>;
  using View =
    eastl::conditional_t<gpu, eastl::conditional_t<eastl::is_same_v<Sbuffer, ResNoConst>, BufferView, TextureView>, BlobView>;
  using GetReturnType = eastl::conditional_t<gpu, ResNoConst *, Res *>;
  VirtualResourceHandle(detail::ResUid resId, const ResourceProvider *prov) : Base{resId, prov} {}

public:
  /**
   * \brief Returns a reference to the provided resource. Only defined
   * for mandatory handles.
   * \details For read requests, this always returns a const reference.
   * Due to unfortunate coupling of textures with samplers, sometimes
   * this constness needs to be violated. Use `.view()` for that.
   *
   * \return A non-nullable reference to the resource.
   */
  REQUIRE(!optional)
  Res &ref() const & { return *get(); }

  /**
   * \brief Returns a pointer to the provided resource, or nullptr for
   * missing optional resources. Always check for null when using this!
   *
   * \return A pointer to the resource that may be nullptr for optional
   *   requests.
   */
  GetReturnType get() const &
  {
    if constexpr (eastl::is_base_of_v<BaseTexture, ResNoConst>)
      return Base::getResourceData<BaseTexture *>();
    else if constexpr (eastl::is_base_of_v<Sbuffer, ResNoConst>)
      return Base::getResourceData<Sbuffer *>();
    else
    {
      auto view = Base::getResourceData<BlobView>();
      G_ASSERT((view.data != nullptr && view.typeTag == tag_for<ResNoConst>()) ||
               (view.data == nullptr && view.typeTag == ResourceSubtypeTag::Invalid)); // sanity check
      return reinterpret_cast<ResNoConst *>(view.data);
    }
  }

  /**
   * \brief Returns view to a provided GPU resource, or an
   * empty view for missing optional resources.
   *
   * \return View to the GPU resource that may be empty for
   *   optional requests.
   */
  REQUIRE(gpu)
  View view() const &
  {
    if constexpr (eastl::is_base_of_v<BaseTexture, ResNoConst>)
      return Base::getResourceData<BaseTexture *>();
    else if constexpr (eastl::is_base_of_v<Sbuffer, ResNoConst>)
      return Base::getResourceData<Sbuffer *>();
    else
      return Base::getResourceData<BlobView>();
  }
};

#undef REQUIRE

} // namespace dafg
