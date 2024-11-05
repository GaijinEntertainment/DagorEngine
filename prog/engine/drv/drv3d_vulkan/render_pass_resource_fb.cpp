// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render_pass_resource.h"
#include <perfMon/dag_statDrv.h>
#include "front_render_pass_state.h"
#include "globals.h"
#include "driver_config.h"

using namespace drv3d_vulkan;

// internal FB storage & managment impl

VulkanFramebufferHandle RenderPassResource::compileOrGetFB()
{
  // be aware of quite memory hungry linear search here!
  constexpr uint32_t maxArraySizeForLinearSearch = 10;
  if (compiledFBs.size() >= maxArraySizeForLinearSearch)
    D3D_ERROR("vulkan: too much FB variations for RP %p[%p]<%s>, reduce them or redo search", this, getBaseHandle(), getDebugName());

  VulkanFramebufferHandle ret;
  uint32_t lastFreeFB = -1;
  VkExtent2D areaExtent = toVkFbExtent(state->area.data);
  for (uint32_t i = 0; i < compiledFBs.size(); ++i)
  {
    if (is_null(compiledFBs[i].handle))
    {
      lastFreeFB = i;
      continue;
    }
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

        if (!fit)
          break;
      }
    if (fit)
    {
      ret = compiledFBs[i].handle;
      break;
    }
  }

  if (is_null(ret))
  {
    FbWithCreationInfo &newFB = lastFreeFB != -1 ? compiledFBs[lastFreeFB] : compiledFBs.push_back();
    for (uint32_t i = 0; i < desc.targetCount; ++i)
    {
      const StateFieldRenderPassTarget &tgt = state->targets.data[i];
      G_ASSERTF(tgt.image != nullptr, "vulkan: attachment %u of RP %p[%p]<%s> is not specified (null)", i, this, getBaseHandle(),
        getDebugName());
      if (Globals::cfg.has.imagelessFramebuffer)
      {
        newFB.atts[i].imageless.width = bakedAttachments->infos[i].width;
        newFB.atts[i].imageless.height = bakedAttachments->infos[i].height;
        newFB.atts[i].imageless.flags = bakedAttachments->infos[i].flags;
        newFB.atts[i].imageless.usage = bakedAttachments->infos[i].usage;
        newFB.atts[i].imageless.layerCount = bakedAttachments->infos[i].layerCount;
        newFB.atts[i].imageless.fmt = tgt.image->getFormat();
      }
      else
        newFB.atts[i].img = tgt.image;
      newFB.atts[i].mipLayer = (tgt.mipLevel << 16) | (tgt.layer & 0xFFFF);

      // verify various stuff

      bool formatsCompatible = false;
      FormatStore expectedFormat = desc.targetFormats[i];
      VkFormat expectedVkFormat = expectedFormat.asVkFormat();
      VkFormat attVkFormat = tgt.image->getFormat().asVkFormat();
      const auto &formatList = bakedAttachments->formatLists[i];

      if (Globals::cfg.has.imagelessFramebuffer)
      {
        for (uint32_t j = 0; j < formatList.size(); ++j)
          formatsCompatible |= expectedVkFormat == formatList[j];
      }
      else
        formatsCompatible |= expectedVkFormat == attVkFormat;

      if (!formatsCompatible)
      {
        String actualFormats("[");
        if (Globals::cfg.has.imagelessFramebuffer)
        {
          for (uint32_t j = 0; j < formatList.size(); ++j)
          {
            actualFormats += FormatStore::fromVkFormat(formatList[j]).getNameString();
            if (j + 1 < formatList.size())
              actualFormats += ", ";
          }
        }
        else
          actualFormats += tgt.image->getFormat().getNameString();
        actualFormats += "]";
        DAG_FATAL("vulkan: attachment %u of RP %p[%p]<%s> expected format %s, got some of %s in image %p<%s>", i, this,
          getBaseHandle(), getDebugName(), expectedFormat.getNameString(), actualFormats, tgt.image, tgt.image->getDebugName());
      }
    }
    newFB.extent = areaExtent;
    newFB.handle = compileFB();
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
        VULKAN_LOG_CALL(device.vkDestroyFramebuffer(device.get(), compiledFBs[i].handle, nullptr));
        compiledFBs[i].handle = VulkanFramebufferHandle();
        break;
      }
    }
  }
}

VulkanFramebufferHandle RenderPassResource::compileFB()
{
  // FB compile can be time consuming, and yet we not clearly state this for user
  // so make a mark for it just in case user somehow ends up in multiple/realtime FB compilations
  TIME_PROFILE(vulkan_native_rp_fb_compile);

  VkFramebufferCreateInfo fbci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr};
  fbci.renderPass = getHandle();
  VkExtent2D areaExtent = toVkFbExtent(state->area.data);
  G_ASSERTF(areaExtent.width && areaExtent.height, "vulkan: render area %u x %u is invalid", areaExtent.width, areaExtent.height);
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
  VULKAN_EXIT_ON_FAIL(device.vkCreateFramebuffer(device.get(), &fbci, nullptr, ptr(fbh)));
  return fbh;
}
