// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// container for dummy resources used for nullptr replacement in descriptor sets
// as vulkan does not handle this situation well

#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <any_descriptor.h>
#include "image_resource.h"
#include "buffer_resource.h"
#if VULKAN_HAS_RAYTRACING
#include "raytrace_as_resource.h"
#endif
#include "drv/3d/dag_texFlags.h"

namespace drv3d_vulkan
{

struct ResourceDummyRef
{
  VkAnyDescriptorInfo descriptor;
  void *resource;
};

typedef ResourceDummyRef ResourceDummySet[spirv::MISSING_INDEX_COUNT];

class DeviceContext;

class DummyResources
{
  static constexpr uint32_t tex_array_count = 6 * 2;
  static constexpr uint32_t tex_dim_size = 1;
  static constexpr uint32_t buf_dummy_elements = 1024 * 8;
  static constexpr uint32_t buf_view_fmt_flag = TEXFMT_A32B32G32R32F;

  ResourceDummySet table = {};
  struct
  {
    Image *srv2D = nullptr;
    Image *srv2Dcompare = nullptr;
    Image *srv3D = nullptr;
    Image *srv3Dcompare = nullptr;
    Image *uav2D = nullptr;
    Image *uav3D = nullptr;
  } img;
  struct
  {
    Buffer *srv = nullptr;
    Buffer *uav = nullptr;
  } buf;
#if VULKAN_HAS_RAYTRACING
  RaytraceAccelerationStructure *tlas = nullptr;
#endif
  const SamplerInfo *sampler = nullptr;
  const SamplerInfo *cmpSampler = nullptr;
  VkDeviceSize dummySize = 0;

  DummyResources(const DummyResources &) = delete;
  DummyResources &operator=(const DummyResources &) = delete;
  Image *createImage(const Image::Description::TrimmedCreateInfo &ii, bool needCompare);

  void createTLAS();
  void createImages();
  void createBuffers();
  void calcDummySize();
  void setObjectNames();
  void uploadZeroContent();
  void fillTable();
  void fillTableImg(uint32_t idx, Image *image, ImageViewState ivs, VkImageLayout layout, VulkanSamplerHandle sampler_handle);
  void fillTableBuf(uint32_t idx, Buffer *buffer, VkDeviceSize sz);

public:
  DummyResources() = default;
  Image *getSRV2DImage() { return img.srv2D; }
  const ResourceDummySet &getTable() const { return table; }

  void init();
  void shutdown(DeviceContext &ctx);
};

} // namespace drv3d_vulkan
