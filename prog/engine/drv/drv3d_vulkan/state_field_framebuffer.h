// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// fields that define framebuffer

#include <drv/3d/dag_decl.h>
#include <math/dag_e3dColor.h>

#include "util/tracked_state.h"
#include "driver.h"
#include "image_view_state.h"

namespace drv3d_vulkan
{

class Image;

struct StateFieldFramebufferAttachment
{
  struct RawFrontData
  {
    BaseTexture *tex;
    int mip;
    int layer;
  };

  using Indexed = TrackedStateIndexedDataRORef<StateFieldFramebufferAttachment>;
  using RawIndexed = TrackedStateIndexedDataRORef<RawFrontData>;

  Image *image;
  ImageViewState view;
  bool useSwapchain;
  BaseTexture *tex;

  static StateFieldFramebufferAttachment empty;
  static StateFieldFramebufferAttachment back_buffer;

  template <typename T>
  void reset(T &)
  {
    image = nullptr;
    view = {};
    useSwapchain = false;
    tex = nullptr;
  }

  void set(const StateFieldFramebufferAttachment &v);
  bool diff(const StateFieldFramebufferAttachment &v) const;

  void set(const RawFrontData &v);
  bool diff(const RawFrontData &v) const;

  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldFramebufferAttachments : TrackedStateFieldArray<StateFieldFramebufferAttachment,
                                            Driver3dRenderTarget::MAX_SIMRT + 1, //+1 for depth/stencil
                                            true, true>
{};

struct StateFieldFramebufferSwapchainSrgbWrite : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<bool>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldFramebufferReadOnlyDepth : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<bool>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldFramebufferClearColor : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<E3DCOLOR>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldFramebufferClearDepth : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<float>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldFramebufferClearStencil : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<uint8_t>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

} // namespace drv3d_vulkan
