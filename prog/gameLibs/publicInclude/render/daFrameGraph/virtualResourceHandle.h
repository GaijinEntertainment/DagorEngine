//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>

#include <render/daFrameGraph/detail/virtualResourceHandleBase.h>

#include <3d/dag_resPtr.h>


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
  using View = eastl::conditional_t<gpu, ManagedResView<ManagedRes<ResNoConst>>, BlobView>;

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
   * \details For read requests, this always returns a const reference.
   * Due to unfortunate coupling of textures with samplers, sometimes
   * this constness needs to be violated. Use `.view()` for that.
   *
   * \return A pointer to the resource that may be nullptr for optional
   *   requests.
   */
  Res *get() const &
  {
    if constexpr (eastl::is_base_of_v<BaseTexture, ResNoConst>)
      return Base::getResourceView<View>().getBaseTex();
    else if constexpr (eastl::is_base_of_v<Sbuffer, ResNoConst>)
      return Base::getResourceView<View>().getBuf();
    else
    {
      auto view = Base::getResourceView<View>();
      G_ASSERT((view.data != nullptr && view.typeTag == tag_for<ResNoConst>()) ||
               (view.data == nullptr && view.typeTag == ResourceSubtypeTag::Invalid)); // sanity check
      return reinterpret_cast<ResNoConst *>(view.data);
    }
  }

  /**
   * \brief Returns a managed view to a provided GPU resource, or an
   * empty view for missing optional resources.
   *
   * \return A ManagedResView to the GPU resource that may be empty for
   *   optional requests.
   */
  REQUIRE(gpu)
  View view() const & { return Base::getResourceView<View>(); }

  /**
   * \brief Returns the engine resource manager ID for a provided GPU
   * resource, or a BAD_RESID for a missing optional resource.
   *
   * \return A D3DRESID to the GPU resource that may be BAD_RESID for
   *   optional requests.
   */
  REQUIRE(gpu)
  D3DRESID d3dResId() const &
  {
    // Pet peeve: why tf do different managed res holders have different
    // getter calls but no common one for templated cases?
    if constexpr (eastl::is_base_of_v<BaseTexture, ResNoConst>)
      return Base::getResourceView<View>().getTexId();
    else if constexpr (eastl::is_base_of_v<Sbuffer, ResNoConst>)
      return Base::getResourceView<View>().getBufId();
    else
      return BAD_D3DRESID;
  }
};

#undef REQUIRE

} // namespace dafg
