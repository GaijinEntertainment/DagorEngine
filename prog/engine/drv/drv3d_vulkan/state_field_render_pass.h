// fields that define rende pass state
#pragma once
#include <3d/dag_drvDecl.h>
#include "util/tracked_state.h"
#include "driver.h"

namespace drv3d_vulkan
{

class Image;
class RenderPassResource;

struct StateFieldRenderPassTarget
{
  using Indexed = TrackedStateIndexedDataRORef<StateFieldRenderPassTarget>;
  using IndexedRaw = TrackedStateIndexedDataRORef<RenderPassTarget>;

  Image *image;
  uint32_t mipLevel;
  uint32_t layer;
  ResourceClearValue clearValue;

  static StateFieldRenderPassTarget empty;

  template <typename T>
  void reset(T &)
  {
    image = nullptr;
    mipLevel = 0;
    layer = 0;
    clearValue = {};
  }

  void set(const StateFieldRenderPassTarget &v);
  void set(const RenderPassTarget &v);
  bool diff(const RenderPassTarget &v) const;
  bool diff(const StateFieldRenderPassTarget &v) const;

  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldRenderPassTargets : TrackedStateFieldArray<StateFieldRenderPassTarget, MAX_RENDER_PASS_ATTACHMENTS, true, true>
{};

struct StateFieldRenderPassSubpassIdx : TrackedStateFieldBase<true, false>
{
  static constexpr uint32_t InvalidSubpass = -1;
  uint32_t data;

  struct Next
  {};
  struct Zero
  {};
  struct Invalid
  {};

  template <typename StorageType>
  void reset(StorageType &)
  {
    data = InvalidSubpass;
  }

  bool diff(Invalid) const { return data != InvalidSubpass; }
  bool diff(Zero) const { return true; }
  bool diff(Next) const { return true; }

  void set(Invalid) { data = InvalidSubpass; }
  void set(Zero)
  {
    G_ASSERTF(data == InvalidSubpass, "vulkan: trying to reset RP subpass inside active render pass");
    data = 0;
  }
  void set(Next)
  {
    G_ASSERTF(data != InvalidSubpass, "vulkan: trying to increment RP subpass without active render pass");
    ++data;
  }
  void set(uint32_t v) { data = v; }

  const uint32_t &getValueRO() const { return data; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldRenderPassResource : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPtr<RenderPassResource>
{
  using PtrType = RenderPassResource *;
  const PtrType &getValueRO() const { return ptr; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldRenderPassArea : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPOD<RenderPassArea>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();

  bool diff(const RenderPassArea &v) const
  {
    return data.left != v.left || data.top != v.top || data.width != v.width || data.height != v.height || data.minZ != v.minZ ||
           data.maxZ != v.maxZ;
  }
  const RenderPassArea &getValueRO() const { return data; }
};

} // namespace drv3d_vulkan
