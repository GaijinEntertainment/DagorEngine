// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render_pass_resource.h"
#include <perfMon/dag_statDrv.h>
#include "front_render_pass_state.h"
#include "globals.h"
#include "driver_config.h"
#include "vulkan_allocation_callbacks.h"
#include "backend.h"
#include "timeline_latency.h"
#include "timelines.h"
#include <util/dag_hash.h>

using namespace drv3d_vulkan;

// internal FB storage & managment impl

VulkanFramebufferHandle RenderPassResource::compileOrGetFB()
{
  // be aware of quite memory hungry linear search here!
  constexpr uint32_t maxArraySizeForLinearSearch = 64;

  VulkanFramebufferHandle ret;
  uint32_t lastFreeFB = -1;
  VkExtent2D areaExtent = toVkFbExtent(state->area.data);

  uint64_t computedFBHash = -1;
  {
    uint64_t hash = FNV1Params<64>::offset_basis;

    hash = fnv1a_step<64>(areaExtent.width, hash);
    hash = fnv1a_step<64>(areaExtent.height, hash);
    for (int i = 0; i < desc.targetCount; ++i)
    {
      const auto &targetData = state->targets.data[i];

      hash = fnv1a_step<64>(targetData.mipLevel, hash);
      hash = fnv1a_step<64>(targetData.layer, hash);
      if (Globals::cfg.has.imagelessFramebuffer)
      {
#if VK_KHR_imageless_framebuffer
        const auto &bakedAttachmentInfo = bakedAttachments->infos[i];
        hash = fnv1a_step<64>(bakedAttachmentInfo.width, hash);
        hash = fnv1a_step<64>(bakedAttachmentInfo.height, hash);
        hash = fnv1a_step<64>(bakedAttachmentInfo.usage, hash);
        hash = fnv1a_step<64>(bakedAttachmentInfo.flags, hash);
        hash = fnv1a_step<64>(bakedAttachmentInfo.layerCount, hash);
        FormatStore imageFormat = targetData.image->getFormat();
        G_STATIC_ASSERT(eastl::has_unique_object_representations_v<FormatStore>);
        hash = mem_hash_fnv1<64>((const char *)(&imageFormat), sizeof(FormatStore), hash, fnv1a_step<64>);
#endif
      }
      else
      {
        hash = mem_hash_fnv1<64>((const char *)(&targetData.image), sizeof(Image *), hash, fnv1a_step<64>);
      }
    }
    computedFBHash = hash;
  }

  for (uint32_t i = 0; i < compiledFBs.size(); ++i)
  {
    if (is_null(compiledFBs[i].handle))
    {
      lastFreeFB = i;
      continue;
    }
    if (computedFBHash != compiledFBHashes[i])
      continue;
#if RP_RESOURCE_CHECK_FRAMEBUFFER_HASH
    bool fit = true;
    fit &= (areaExtent.height == compiledFBs[i].extent.height) && (areaExtent.width == compiledFBs[i].extent.width);
    if (fit)
      for (uint32_t j = 0; j < desc.targetCount; ++j)
      {
        const FbWithCreationInfo::CompressedAtt &att = compiledFBs[i].atts[j];
        fit &= (state->targets.data[j].layer == (att.mipLayer & 0xFFFF));
        fit &= (state->targets.data[j].mipLevel == ((att.mipLayer >> 16) & 0xFF));

        if (Globals::cfg.has.imagelessFramebuffer)
        {
#if VK_KHR_imageless_framebuffer
          fit &= bakedAttachments->infos[j].width == att.imageless.width;
          fit &= bakedAttachments->infos[j].height == att.imageless.height;
          fit &= bakedAttachments->infos[j].usage == att.imageless.usage;
          fit &= bakedAttachments->infos[j].flags == att.imageless.flags;
          fit &= bakedAttachments->infos[j].layerCount == att.imageless.layerCount;
          fit &= state->targets.data[j].image->getFormat() == att.imageless.fmt;
#endif
        }
        else
          fit &= state->targets.data[j].image == att.img;

        G_ASSERT(fit);

        if (!fit)
          break;
      }
    if (fit)
#endif
    {
      compiledFBs[i].lastUsedGpuWorkId = Backend::gpuJob->index;
      ret = compiledFBs[i].handle;
      break;
    }
  }

  if (is_null(ret))
  {
    if (lastFreeFB == -1 && compiledFBs.size() == maxArraySizeForLinearSearch)
    {
      // evict least recently used entry by GPU work ID
      uint32_t lruIdx = 0;
      size_t oldestWorkId = compiledFBs[0].lastUsedGpuWorkId;
      for (uint32_t i = 1; i < compiledFBs.size(); ++i)
      {
        if (compiledFBs[i].lastUsedGpuWorkId < oldestWorkId)
        {
          oldestWorkId = compiledFBs[i].lastUsedGpuWorkId;
          lruIdx = i;
        }
      }
      VulkanDevice &device = Globals::VK::dev;
      if (TimelineLatency::isGPUWorkCompleted(oldestWorkId, Backend::gpuJob->index) && !is_null(compiledFBs[lruIdx].handle))
      {
        VULKAN_LOG_CALL(device.vkDestroyFramebuffer(device.get(), compiledFBs[lruIdx].handle, VKALLOC(framebuffer)));
        compiledFBs[lruIdx].handle = VulkanFramebufferHandle();
        lastFreeFB = lruIdx;
      }
    }

    uint64_t &newFBHash = lastFreeFB != -1 ? compiledFBHashes[lastFreeFB] : compiledFBHashes.push_back();
    newFBHash = computedFBHash;

    FbWithCreationInfo &newFB = lastFreeFB != -1 ? compiledFBs[lastFreeFB] : compiledFBs.push_back();
    if (compiledFBs.size() > maxArraySizeForLinearSearch && lastFreeFB == -1)
      D3D_ERROR("vulkan: too much FB variations for RP %p[%p]<%s>, reduce them or redo search", this, getBaseHandle(), getDebugName());
    newFB.lastUsedGpuWorkId = Backend::gpuJob->index;

    G_ASSERT(compiledFBHashes.size() == compiledFBs.size());

    for (uint32_t i = 0; i < desc.targetCount; ++i)
    {
      const StateFieldRenderPassTarget &tgt = state->targets.data[i];
      D3D_CONTRACT_ASSERTF(tgt.image != nullptr, "vulkan: attachment %u of RP %p[%p]<%s> is not specified (null)", i, this,
        getBaseHandle(), getDebugName());
      if (Globals::cfg.has.imagelessFramebuffer)
      {
        newFB.atts[i].imageless.usage = bakedAttachments->infos[i].usage;
#if RP_RESOURCE_CHECK_FRAMEBUFFER_HASH
        newFB.atts[i].imageless.width = bakedAttachments->infos[i].width;
        newFB.atts[i].imageless.height = bakedAttachments->infos[i].height;
        newFB.atts[i].imageless.flags = bakedAttachments->infos[i].flags;
        newFB.atts[i].imageless.layerCount = bakedAttachments->infos[i].layerCount;
        newFB.atts[i].imageless.fmt = tgt.image->getFormat();
#endif
      }
      else
        newFB.atts[i].img = tgt.image;
#if RP_RESOURCE_CHECK_FRAMEBUFFER_HASH
      newFB.atts[i].mipLayer = (tgt.mipLevel << 16) | (tgt.layer & 0xFFFF);
#endif

      // verify various stuff

      bool formatsCompatible = false;
      FormatStore expectedFormat = desc.targetFormats[i];
      VkFormat expectedVkFormat = expectedFormat.asVkFormat();
      Image::ViewFormatList formatList;

      if (Globals::cfg.has.imagelessFramebuffer)
      {
        formatList = bakedAttachments->formatLists[i];
        for (uint32_t j = 0; j < formatList.size(); ++j)
          formatsCompatible |= expectedVkFormat == formatList[j];
      }
      else
      {
        viewFormatListFrom(tgt.image->getFormat(), tgt.image->getUsage(), formatList);
        for (uint32_t j = 0; j < formatList.size(); ++j)
          formatsCompatible |= expectedVkFormat == formatList[j];
      }

      if (!formatsCompatible)
      {
        String actualFormats("[");
        for (uint32_t j = 0; j < formatList.size(); ++j)
        {
          actualFormats += FormatStore::fromVkFormat(formatList[j]).getNameString();
          if (j + 1 < formatList.size())
            actualFormats += ", ";
        }
        actualFormats += "]";
        DAG_FATAL("vulkan: attachment %u of RP %p[%p]<%s> expected format %s, got some of %s in image %p<%s>", i, this,
          getBaseHandle(), getDebugName(), expectedFormat.getNameString(), actualFormats, tgt.image, tgt.image->getDebugName());
      }
    }
#if RP_RESOURCE_CHECK_FRAMEBUFFER_HASH
    newFB.extent = areaExtent;
#endif
    newFB.handle = compileFB(newFB);
    ret = newFB.handle;
  }

  return ret;
}

void RenderPassResource::destroyFBsWithImage(const Image *img)
{
  if (Globals::cfg.has.imagelessFramebuffer)
    return; // with this extension image is not bound to framebuffer

  VulkanDevice &device = Globals::VK::dev;
  for (uint32_t i = 0; i < compiledFBs.size(); ++i)
  {
    if (is_null(compiledFBs[i].handle))
      continue;

    for (uint32_t j = 0; j < desc.targetCount; ++j)
    {
      if (compiledFBs[i].atts[j].img == img)
      {
        VULKAN_LOG_CALL(device.vkDestroyFramebuffer(device.get(), compiledFBs[i].handle, VKALLOC(framebuffer)));
        compiledFBs[i].handle = VulkanFramebufferHandle();
        break;
      }
    }
  }
}

VulkanFramebufferHandle RenderPassResource::compileFB(const FbWithCreationInfo &ref_info)
{
  // FB compile can be time consuming, and yet we not clearly state this for user
  // so make a mark for it just in case user somehow ends up in multiple/realtime FB compilations
  TIME_PROFILE(vulkan_native_rp_fb_compile);

  VkFramebufferCreateInfo fbci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr};
  fbci.renderPass = getHandle();
  VkExtent2D areaExtent = toVkFbExtent(state->area.data);
  D3D_CONTRACT_ASSERTF(areaExtent.width && areaExtent.height, "vulkan: render area %u x %u is invalid", areaExtent.width,
    areaExtent.height);
  fbci.width = areaExtent.width;
  fbci.height = areaExtent.height;
  fbci.attachmentCount = desc.targetCount;
  fbci.layers = 1;

  // amount of layers should be limited to minimal layers value in attachment set
  uint32_t minLayers = UINT32_MAX;
  for (uint32_t i = 0; i < desc.targetCount; ++i)
    minLayers = min<uint32_t>(bakedAttachments->layerCounts[i], minLayers);
  if (UINT32_MAX != minLayers)
    fbci.layers = minLayers;

#if VK_KHR_imageless_framebuffer
  VkFramebufferAttachmentsCreateInfoKHR fbaci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO_KHR, nullptr};
  if (Globals::cfg.has.imagelessFramebuffer)
  {
    fbci.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT_KHR;
    for (uint32_t i = 0; i < desc.targetCount; ++i)
    {
      // TODO: do it once!
      bakedAttachments->infos[i].usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
      bakedAttachments->infos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO_KHR;
      bakedAttachments->infos[i].pNext = nullptr;
      bakedAttachments->infos[i].viewFormatCount = bakedAttachments->formatLists[i].size();
      bakedAttachments->infos[i].pViewFormats = bakedAttachments->formatLists[i].data();
    }
    fbaci.attachmentImageInfoCount = desc.targetCount;
    fbaci.pAttachmentImageInfos = &bakedAttachments->infos[0];
    chain_structs(fbci, fbaci);
  }
  else
#endif
    fbci.pAttachments = ary(&bakedAttachments->views[0]);

  VulkanDevice &device = Globals::VK::dev;
  VulkanFramebufferHandle fbh;
  VULKAN_EXIT_ON_FAIL(device.vkCreateFramebuffer(device.get(), &fbci, VKALLOC(framebuffer), ptr(fbh)));

#if VK_KHR_imageless_framebuffer
  // restore patched usage to original
  if (Globals::cfg.has.imagelessFramebuffer)
  {
    for (uint32_t i = 0; i < desc.targetCount; ++i)
      bakedAttachments->infos[i].usage = ref_info.atts[i].imageless.usage;
  }
#endif

  return fbh;
}
