// Copyright (C) Gaijin Games KFT.  All rights reserved.

// texformat query and helpers implementation
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_texture.h>
#include "format_store.h"
#include "globals.h"
#include "vk_format_utils.h"
#include "driver_config.h"

using namespace drv3d_vulkan;

namespace
{

static VkImageUsageFlags usage_flags_from_cfg(int cflg, FormatStore fmt)
{
  VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  if (cflg & TEXCF_RTARGET)
  {
    if (fmt.getAspektFlags() & VK_IMAGE_ASPECT_COLOR_BIT)
      flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else
      flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  if (cflg & TEXCF_UNORDERED)
  {
    flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  }
  if (!(cflg & TEXCF_SYSMEM))
  {
    flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  return flags;
}

} // anonymous namespace

bool d3d::check_texformat(int cflg)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  return Globals::VK::fmt.support(fmt.asVkFormat(), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage_flags_from_cfg(cflg, fmt), 0,
    VkSampleCountFlagBits(get_sample_count(cflg)));
}

int d3d::get_max_sample_count(int cflg)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  if (auto sampleFlags =
        Globals::VK::fmt.samples(fmt.asVkFormat(), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage_flags_from_cfg(cflg, fmt)))
  {
    for (int samples = get_sample_count(TEXCF_SAMPLECOUNT_MAX); samples; samples >>= 1)
    {
      if (sampleFlags & samples)
        return samples;
    }
  }

  return 1;
}

bool d3d::issame_texformat(int cflg1, int cflg2)
{
  auto formatA = FormatStore::fromCreateFlags(cflg1);
  auto formatB = FormatStore::fromCreateFlags(cflg2);
  return formatA.asVkFormat() == formatB.asVkFormat();
}

bool d3d::check_cubetexformat(int cflg)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  return Globals::VK::fmt.support(fmt.asVkFormat(), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage_flags_from_cfg(cflg, fmt),
    VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VkSampleCountFlagBits(get_sample_count(cflg)));
}

bool d3d::check_voltexformat(int cflg)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  VkImageCreateFlags flags = 0;
  if (cflg & TEXCF_RTARGET)
  {
    if (Globals::cfg.has.renderTo3D)
      flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
    else
      return false;
  }
  return Globals::VK::fmt.support(fmt.asVkFormat(), VK_IMAGE_TYPE_3D, VK_IMAGE_TILING_OPTIMAL, usage_flags_from_cfg(cflg, fmt), flags,
    VkSampleCountFlagBits(get_sample_count(cflg)));
}

unsigned d3d::get_texformat_usage(int cflg, D3DResourceType type)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  VkFormatFeatureFlags features = Globals::VK::fmt.features(fmt.asVkFormat());

  unsigned result = 0;
  switch (type)
  {
    case D3DResourceType::TEX:
    case D3DResourceType::CUBETEX:
    case D3DResourceType::ARRTEX:
    case D3DResourceType::VOLTEX:
      if (features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
      {
        result |= USAGE_RTARGET;
      }
      if (features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
      {
        result |= USAGE_BLEND;
      }
      if (features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      {
        result |= USAGE_DEPTH;
      }
      if (features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
      {
        result |= USAGE_TEXTURE;
        result |= USAGE_VERTEXTEXTURE;
        if (features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
          result |= USAGE_FILTER;
      }
      if (features & (VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
        result |= USAGE_UNORDERED | USAGE_UNORDERED_LOAD;
      break;
    case D3DResourceType::CUBEARRTEX: break;
    case D3DResourceType::SBUF: break;
  }
  return result | USAGE_PIXREADWRITE;
}
