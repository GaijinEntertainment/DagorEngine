#include "device.h"

#include <EASTL/algorithm.h>

using namespace drv3d_vulkan;

#if VERBOSE_PASS_BUILD
#define PASS_BUILD_INFO debug
#else
#define PASS_BUILD_INFO(...)
#endif

VkExtent2D RenderPassClass::FramebufferDescription::makeDrawArea(VkExtent2D def /*= {}*/)
{
  // if swapchain for 0 is used we need to check depth stencil use,
  // in some cases depth stencil was used with swapchain where it
  // was smaller than the swapchain images and crashes.
  if (colorAttachments[0].img)
    return colorAttachments[0].img->getMipExtents2D(colorAttachments[0].view.getMipBase());

  for (int i = 1; i < array_size(colorAttachments); ++i)
    if (colorAttachments[i].img)
      return colorAttachments[i].img->getMipExtents2D(colorAttachments[i].view.getMipBase());

  if (depthStencilAttachment.img)
    return depthStencilAttachment.img->getMipExtents2D(depthStencilAttachment.view.getMipBase());

  return def;
}

VulkanRenderPassHandle RenderPassClass::getPass(VulkanDevice &device, int clear_mask)
{
  uint32_t index = encode_variant(clear_mask);
  G_ASSERT(index < NUM_PASS_VARIANTS);
  if (is_null(variants[index]))
    return compileVariant(device, clear_mask);
  return variants[index];
}
VulkanFramebufferHandle RenderPassClass::getFrameBuffer(VulkanDevice &device, const FramebufferDescription &info)
{
  bool imageless = get_device().hasImagelessFramebuffer();
  auto ref = eastl::find_if(begin(frameBuffers), end(frameBuffers), [&info, &imageless](const FrameBuffer &fb) {
    return imageless ? fb.desc.compatibleToImageless(info) : fb.desc.compatibleTo(info);
  });
  if (ref != end(frameBuffers))
    return ref->frameBuffer;

  return compileFrameBuffer(device, info);
}

RenderPassClass::RenderPassClass(const Identifier &id) : identifier(id) {}

void RenderPassClass::unloadAll(VulkanDevice &device)
{
  for (auto &&variant : variants)
  {
    if (!is_null(variant))
    {
      VULKAN_LOG_CALL(device.vkDestroyRenderPass(device.get(), variant, nullptr));
      variant = VulkanRenderPassHandle{};
    }
  }

  for (auto &&fb : frameBuffers)
    VULKAN_LOG_CALL(device.vkDestroyFramebuffer(device.get(), fb.frameBuffer, nullptr));
  frameBuffers.clear();
}

void RenderPassClass::unloadWith(VulkanDevice &device, VulkanImageViewHandle view)
{
#if VK_KHR_imageless_framebuffer
  if (get_device().hasImagelessFramebuffer())
    return; // With this extension the framebuffer is not bound to the view
#endif
  auto newEnd = eastl::remove_if(begin(frameBuffers), end(frameBuffers),
    [&device, view](const FrameBuffer &fb) //
    {
      if (fb.desc.contains(view))
      {
        VULKAN_LOG_CALL(device.vkDestroyFramebuffer(device.get(), fb.frameBuffer, nullptr));
        return true;
      }
      return false;
    });
  frameBuffers.erase(newEnd, end(frameBuffers));
}

StaticTab<VkClearValue, Driver3dRenderTarget::MAX_SIMRT + 1> RenderPassClass::constructClearValueSet(const VkClearColorValue *ccv,
  const VkClearDepthStencilValue &cdsv) const
{
  StaticTab<VkClearValue, Driver3dRenderTarget::MAX_SIMRT + 1> result;
  if (auto usedColorMask = identifier.colorTargetMask)
  {
    for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT && usedColorMask; ++i, usedColorMask >>= 1)
      if (0 != (usedColorMask & 1))
        result.push_back().color = ccv[i];
  }
  if (identifier.depthState != RenderPassClass::Identifier::NO_DEPTH)
    result.push_back().depthStencil = cdsv;
  return result;
}

bool RenderPassClass::verifyPass(int clear_mask)
{
  const bool clearColor = (clear_mask & CLEAR_TARGET) != 0;
  if (auto usedColorMask = identifier.colorTargetMask)
  {
    for (int i = 0; (i < Driver3dRenderTarget::MAX_SIMRT) && usedColorMask; ++i, usedColorMask >>= 1)
    {
      if ((usedColorMask & 1) == 0)
        continue;

      bool isMultiSampledAttachment = identifier.colorSamples[i] > 1;

      if (isMultiSampledAttachment && !clearColor)
      {
        logerr("MSAA verifyPass failed for attachment %u", i);
        return false;
      }
    }
  }
  return true;
}

VulkanRenderPassHandle RenderPassClass::compileVariant(VulkanDevice &device, int clear_mask)
{
  auto isClear = [](const VkAttachmentLoadOp load_op) { return load_op == VK_ATTACHMENT_LOAD_OP_CLEAR; };

  auto toLoadOp = [clear_mask](const int clear_value, const int discard_value) {
    if (clear_mask & discard_value)
    {
      return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    if (clear_mask & clear_value)
    {
      return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  };

  VkAttachmentLoadOp colorLoadOp = toLoadOp(CLEAR_TARGET, CLEAR_DISCARD_TARGET);
  VkAttachmentLoadOp depthLoadOp = toLoadOp(CLEAR_ZBUFFER, CLEAR_DISCARD_ZBUFFER);
  VkAttachmentLoadOp stencilLoadOp = toLoadOp(CLEAR_STENCIL, CLEAR_DISCARD_STENCIL);

  bool const_depth_stencil = identifier.depthState == Identifier::RO_DEPTH;
  G_ASSERT(!((isClear(depthLoadOp) || isClear(stencilLoadOp)) && const_depth_stencil));
  PASS_BUILD_INFO("building pass with loadop-color=%s, loadop-depth=%s, loadop-stencil=%s", formatAttachmentLoadOp(colorLoadOp),
    formatAttachmentLoadOp(depthLoadOp), formatAttachmentLoadOp(stencilLoadOp));

  bool useNoStoreOp = get_device().hasAttachmentNoStoreOp();
  VkSubpassDependency selfDept;
  selfDept.srcSubpass = 0;
  selfDept.dstSubpass = 0;
  selfDept.srcStageMask = 0;
  selfDept.dstStageMask = 0;
  selfDept.srcAccessMask = 0;
  selfDept.dstAccessMask = 0;
  selfDept.dependencyFlags = 0;

  StaticTab<VkAttachmentDescription, Driver3dRenderTarget::MAX_SIMRT + 1> attachmentDefs;
  StaticTab<VkAttachmentReference, Driver3dRenderTarget::MAX_SIMRT> colorRef;
  StaticTab<VkAttachmentReference, Driver3dRenderTarget::MAX_SIMRT> resolveRef;
  VkAttachmentReference depthStencilRef;

  VkSubpassDescription subPass;
  subPass.flags = 0;
  subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subPass.inputAttachmentCount = 0;
  subPass.pInputAttachments = nullptr;
  subPass.pResolveAttachments = 0;
  subPass.preserveAttachmentCount = 0;
  subPass.pPreserveAttachments = nullptr;

  if (auto usedColorMask = identifier.colorTargetMask)
  {
    // define minimum slots that are needed for usedColorMask
    for (int i = 0; (i < Driver3dRenderTarget::MAX_SIMRT) && usedColorMask; ++i, usedColorMask >>= 1)
    {
      bool isMultiSampledAttachment = identifier.colorSamples[i] > 1;
      bool isResolveAttachment = (i > 0) && (identifier.colorSamples[i - 1] > 1);

      VkAttachmentReference &ref = isResolveAttachment ? resolveRef.push_back() : colorRef.push_back();
      ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      if (0 != (usedColorMask & 1))
      {
        // skip this check as we discard attachment for forced sample count emulation
        // G_ASSERTF(!isMultiSampledAttachment || ((identifier.colorTargetMask & (1 << (i + 1))) &&
        //                                         (identifier.colorSamples[i + 1] == 1)),
        //           "For every MSAA target, a resolution target need to be specified at the next "
        //           "attachment index");

        G_ASSERTF(!isMultiSampledAttachment || (identifier.colorSamples[0] == identifier.colorSamples[i]),
          "All multisampled attachment must have the same sample count");

        ref.attachment = attachmentDefs.size();

        VkAttachmentDescription &def = attachmentDefs.push_back();
        def.flags = 0;
        def.format = identifier.colorFormats[i].asVkFormat();
        def.samples = VkSampleCountFlagBits(identifier.colorSamples[i]);
        if (isMultiSampledAttachment)
        {
          // we never want to load a multisampled attachment
          def.loadOp = isClear(colorLoadOp) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          def.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        else if (isResolveAttachment)
        {
          def.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          def.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        else
        {
          def.loadOp = colorLoadOp;
          def.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        def.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def.initialLayout = ref.layout;
        def.finalLayout = ref.layout;

        PASS_BUILD_INFO("using color attachment [%u] with format %s", i, identifier.colorFormats[i].getNameString());
      }
      else
      {
        ref.attachment = VK_ATTACHMENT_UNUSED;
        PASS_BUILD_INFO("unused color attachment [%u]", i);
      }
    }
    subPass.colorAttachmentCount = colorRef.size();
    subPass.pColorAttachments = colorRef.data();
    subPass.pResolveAttachments = resolveRef.size() > 0 ? resolveRef.data() : nullptr;
  }
  else
  {
    PASS_BUILD_INFO("with no color attachments");
    subPass.colorAttachmentCount = 0;
    subPass.pColorAttachments = nullptr;
  }
  if (identifier.depthState != RenderPassClass::Identifier::NO_DEPTH)
  {
    depthStencilRef.attachment = attachmentDefs.size();

    // depth barriers

    // v1 - depth RW
    if (!const_depth_stencil)
    {
      depthStencilRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    else if (!useNoStoreOp)
    // v2 - depth RO with store
    {
      depthStencilRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      // internal read to internal write-store self dependency
      selfDept.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT;
      selfDept.srcStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      selfDept.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      selfDept.dstStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      selfDept.dependencyFlags |= VK_DEPENDENCY_BY_REGION_BIT;
    }
    else
    // v3 - depth RO without store
    {
      depthStencilRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    subPass.pDepthStencilAttachment = &depthStencilRef;

    VkAttachmentDescription &desc = attachmentDefs.push_back();
    desc.flags = 0;
    desc.format = identifier.depthStencilFormat.asVkFormat();
    desc.samples = VkSampleCountFlagBits(identifier.colorSamples[0]);

    if (identifier.colorSamples[0] > 1)
    {
      desc.loadOp = depthLoadOp;
      desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    else
    {
      desc.loadOp = depthLoadOp;
      if (const_depth_stencil && useNoStoreOp)
      {
#if VK_EXT_load_store_op_none
        desc.storeOp = VK_ATTACHMENT_STORE_OP_NONE;
#elif VK_QCOM_render_pass_store_ops
        desc.storeOp = VK_ATTACHMENT_STORE_OP_NONE_QCOM;
#else
        ;
#endif
      }
      else
        desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    desc.stencilLoadOp = stencilLoadOp;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    desc.initialLayout = depthStencilRef.layout;
    desc.finalLayout = depthStencilRef.layout;
    PASS_BUILD_INFO("using depth stencil attachment with format %s", identifier.depthStencilFormat.getNameString());
  }
  else
  {
    subPass.pDepthStencilAttachment = nullptr;
    PASS_BUILD_INFO("with no depth stencil attachment");
  }

  VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rpci.attachmentCount = attachmentDefs.size();
  rpci.pAttachments = attachmentDefs.data();
  rpci.subpassCount = 1;
  rpci.pSubpasses = &subPass;
  // don't use self dependency if const DS is not present
  int depsCount = (const_depth_stencil && !useNoStoreOp) ? 1 : 0;
  rpci.dependencyCount = attachmentDefs.size() ? depsCount : 0;
  rpci.pDependencies = &selfDept;

  uint32_t index = encode_variant(clear_mask);

  VULKAN_EXIT_ON_FAIL(device.vkCreateRenderPass(device.get(), &rpci, nullptr, ptr(variants[index])));
  PASS_BUILD_INFO("pass handle is %X", generalize(variants[index]));
  return variants[index];
}

VulkanFramebufferHandle RenderPassClass::compileFrameBuffer(VulkanDevice &device, const FramebufferDescription &info)
{
  VkFramebufferCreateInfo fbci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr};
  fbci.renderPass = getPass(device, 0);
  fbci.width = info.extent.width;
  fbci.height = info.extent.height;
  fbci.layers = 1;

  PASS_BUILD_INFO("vulkan: new FB: %ux%u rp %p", fbci.width, fbci.height, fbci.renderPass);

  // amount of layers should be limited to minimal layers value in attachment set
  uint32_t minLayers = UINT32_MAX;

  // referenced in VkFramebufferCreateInfo by adress so defined in this scope
  StaticTab<VulkanImageViewHandle, Driver3dRenderTarget::MAX_SIMRT + 1> attachmentSet;
#if VK_KHR_imageless_framebuffer
  VkFramebufferAttachmentsCreateInfoKHR fbaci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO_KHR, nullptr};
  StaticTab<VkFramebufferAttachmentImageInfoKHR, Driver3dRenderTarget::MAX_SIMRT + 1> fbaiis;
  StaticTab<Image::ViewFormatList, Driver3dRenderTarget::MAX_SIMRT + 1> viewFormats;

  if (get_device().hasImagelessFramebuffer())
  {
    fbci.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT_KHR;

    fbaiis = identifier.squash<VkFramebufferAttachmentImageInfoKHR>(array_size(info.colorAttachments), info.colorAttachments,
      info.depthStencilAttachment, [&minLayers, &viewFormats](const FramebufferDescription::AttachmentInfo &attachment) {
        minLayers = min<uint32_t>(attachment.imageInfo.layers, minLayers);

        VkFramebufferAttachmentImageInfoKHR fbaii = {VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO_KHR, nullptr};
        fbaii.flags = attachment.flags;
        fbaii.usage = attachment.usage;
        fbaii.width = attachment.imageInfo.width;
        fbaii.height = attachment.imageInfo.height;
        fbaii.layerCount = attachment.imageInfo.layers;

        Image::ViewFormatList &imageViewFormats = viewFormats.push_back();
        viewFormatListFrom(FormatStore(attachment.imageInfo.format), attachment.usage, imageViewFormats);

        fbaii.viewFormatCount = imageViewFormats.size();
        fbaii.pViewFormats = imageViewFormats.data();
        return fbaii;
      });

    fbci.attachmentCount = fbaiis.size();
    fbci.pAttachments = nullptr; // this parameter is ignored with imageless framebuffers

    fbaci.attachmentImageInfoCount = fbaiis.size();
    fbaci.pAttachmentImageInfos = fbaiis.data();

    chain_structs(fbci, fbaci);
  }
  else
#endif
  {
    attachmentSet = identifier.squash<VulkanImageViewHandle>(array_size(info.colorAttachments), info.colorAttachments,
      info.depthStencilAttachment, [&minLayers](const FramebufferDescription::AttachmentInfo &attachment) {
        PASS_BUILD_INFO("vulkan: new FB: attachment view %p", generalize(attachment.imageView));
        minLayers = min<uint32_t>(attachment.imageInfo.layers, minLayers);
        return attachment.viewHandle;
      });

    fbci.attachmentCount = attachmentSet.size();
    fbci.pAttachments = ary(attachmentSet.data());
  }

  if (UINT32_MAX != minLayers)
    fbci.layers = minLayers;

  VulkanFramebufferHandle fbh;
  VULKAN_EXIT_ON_FAIL(device.vkCreateFramebuffer(device.get(), &fbci, nullptr, ptr(fbh)));

  frameBuffers.push_back(FrameBuffer{info, fbh});

  return fbh;
}

RenderPassClass *RenderPassManager::getPassClass(const RenderPassClass::Identifier &id)
{
  auto ref = eastl::find_if(begin(passClasses), end(passClasses),
    [id](const eastl::unique_ptr<RenderPassClass> &rpc) { return id == rpc->getIdentifier(); });
  if (ref != end(passClasses))
    return ref->get();

  passClasses.push_back(eastl::make_unique<RenderPassClass>(id));
  return passClasses.back().get();
}

void RenderPassManager::unloadWith(VulkanDevice &device, VulkanImageViewHandle view)
{
  for (auto &&pc : passClasses)
    pc->unloadWith(device, view);
}

void RenderPassManager::unloadAll(VulkanDevice &device)
{
  for (auto &&pc : passClasses)
    pc->unloadAll(device);
  passClasses.clear();
}

RenderPassClass::FramebufferDescription::AttachmentInfo::AttachmentInfo() : viewHandle{}, imageInfo{}, img{nullptr}, usage(), flags()
{}

RenderPassClass::FramebufferDescription::AttachmentInfo::AttachmentInfo(Image *image, VulkanImageViewHandle view_handle,
  ImageViewState ivs) :
  imageInfo{}, usage(), flags()
{
  img = image;
  view = ivs;
  imageInfo.layers = ivs.isArray ? ivs.getArrayCount() : 1;
  viewHandle = view_handle;

  if (get_device().hasImagelessFramebuffer())
  {
    VkExtent2D viewExtent = image->getMipExtents2D(ivs.getMipBase());
    imageInfo.width = viewExtent.width;
    imageInfo.height = viewExtent.height;
    imageInfo.mipLevel = ivs.getMipBase();
    imageInfo.format = image->getFormat();

    usage = image->getUsage();
    flags = image->getCreateFlags();
  }
}

bool RenderPassClass::FramebufferDescription::contains(VulkanImageViewHandle view) const
{
  if (get_device().hasImagelessFramebuffer())
    return false;
  using namespace eastl;
  return (end(colorAttachments) !=
           find_if(begin(colorAttachments), end(colorAttachments),
             [&view](const FramebufferDescription::AttachmentInfo &info) { return info.viewHandle == view; })) ||
         (depthStencilAttachment.viewHandle == view);
}