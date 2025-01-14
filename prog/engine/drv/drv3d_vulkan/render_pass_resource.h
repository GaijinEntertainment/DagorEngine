// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_relocatableFixedVector.h>
#include <drv/3d/dag_renderPass.h>
#include <osApiWrappers/dag_critSec.h>
#include <atomic>

#include "device_resource.h"
#include "subpass_dependency.h"
#include "image_resource.h"
#include "image_view_state.h"

namespace drv3d_vulkan
{

using RenderPassCreationInfo = RenderPassDesc;
struct StateFieldRenderPassTarget;
struct FrontRenderPassStateStorage;
class VariatedGraphicsPipeline;
class ExecutionContext;
struct PipelineStageStateBase;

struct RenderPassDescription
{
  const RenderPassDesc *externDesc;
  static constexpr int USUAL_MAX_SUBPASSES = 8;
  static constexpr int USUAL_MAX_ATTACHMENTS = 8;
  static constexpr int USUAL_MAX_REFS = 16;
  static constexpr int USUAL_MAX_ASYNC_CREATIONS = 8;

  struct SubpassAttachmentsInfo
  {
    int32_t depthStencilAttachment;
    uint32_t colorWriteMask;
    VkSampleCountFlagBits msaaSamples;
  };

  uint32_t targetCount;
  uint32_t subpasses;
  dag::RelocatableFixedVector<FormatStore, USUAL_MAX_ATTACHMENTS> targetFormats;
  dag::RelocatableFixedVector<SubpassAttachmentsInfo, USUAL_MAX_SUBPASSES> subpassAttachmentsInfos;
  dag::RelocatableFixedVector<VkSubpassDependency, USUAL_MAX_SUBPASSES> selfDeps;
  dag::RelocatableFixedVector<dag::RelocatableFixedVector<uint32_t, USUAL_MAX_ATTACHMENTS>, USUAL_MAX_SUBPASSES> inputAttachments;
  dag::RelocatableFixedVector<dag::RelocatableFixedVector<VkImageLayout, USUAL_MAX_ATTACHMENTS>, USUAL_MAX_SUBPASSES> attImageLayouts;

  using PerAttachmentOps = dag::RelocatableFixedVector<SubpassDep::BarrierScope, USUAL_MAX_ATTACHMENTS>;
  using PerSubpassOps = dag::RelocatableFixedVector<PerAttachmentOps, USUAL_MAX_SUBPASSES>;
  PerSubpassOps attachmentOperations;
  dag::RelocatableFixedVector<bool, USUAL_MAX_ATTACHMENTS> attachmentContentLoaded;

  uint64_t hash;
  bool noOpWithoutDraws;

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
};

typedef ResourceImplBase<RenderPassDescription, VulkanRenderPassHandle, ResourceType::RENDER_PASS> RenderPassResourceImplBase;

class DeviceContext;

// use Resource suffix to avoid confusion with other "RenderPass" stuff
class RenderPassResource : public RenderPassResourceImplBase
{

public:
  // external handlers
  static constexpr int CLEANUP_DESTROY = 0;

  void destroyVulkanObject();
  void createVulkanObject();
  MemoryRequirementInfo getMemoryReq();
  VkMemoryRequirements getSharedHandleMemoryReq();
  void bindMemory();
  void reuseHandle();
  void releaseSharedHandle();
  void evict();
  bool isEvictable();
  void shutdown();
  bool nonResidentCreation();
  void restoreFromSysCopy(ExecutionContext &ctx);
  void makeSysCopy(ExecutionContext &ctx);

  template <int Tag>
  void onDelayedCleanupBackend(){};

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

  struct BakedAttachmentSharedData
  {

    // interleave to avoid transform for creation/bind vk methods
    VkClearValue clearValues[MAX_RENDER_PASS_ATTACHMENTS];
    VulkanImageViewHandle views[MAX_RENDER_PASS_ATTACHMENTS];
    uint32_t layerCounts[MAX_RENDER_PASS_ATTACHMENTS];
#if VK_KHR_imageless_framebuffer
    VkFramebufferAttachmentImageInfoKHR infos[MAX_RENDER_PASS_ATTACHMENTS];
    Image::ViewFormatList formatLists[MAX_RENDER_PASS_ATTACHMENTS];
#endif

    struct PerAttachment
    {
      VkClearValue &clearValue;
      VulkanImageViewHandle &view;
      uint32_t &layers;
#if VK_KHR_imageless_framebuffer
      VkFramebufferAttachmentImageInfoKHR &info;
      Image::ViewFormatList &formats;
#endif

      PerAttachment(BakedAttachmentSharedData &v, uint32_t idx) :
        clearValue(v.clearValues[idx]),
        view(v.views[idx]),
        layers(v.layerCounts[idx])
#if VK_KHR_imageless_framebuffer
        ,
        info(v.infos[idx]),
        formats(v.formatLists[idx])
#endif
      {}
    };
  };

private:
  struct SubpassExtensions
  {
    // index into VkAttachmentReference array
    // -1 means that no depthStencilResolve struct should be chained
    int depthStencilResolveAttachmentIdx = -1;
  };
  struct RenderPassConvertTempData
  {
    Tab<VkSubpassDependency> deps;
    Tab<VkSubpassDependency> depsR;
    Tab<VkSubpassDescription> subpasses;
    Tab<VkAttachmentReference> refs;
    Tab<uint32_t> preserves;
    Tab<VkAttachmentDescription> attachments;
    Tab<SubpassExtensions> subpass_extensions;
    void clear()
    {
      deps.clear();
      depsR.clear();
      subpasses.clear();
      refs.clear();
      preserves.clear();
      attachments.clear();
      subpass_extensions.clear();
    }
  };
  static Tab<RenderPassConvertTempData> tempDataCache;
  static Tab<int> tempDataCacheIndexes;
  static WinCritSec tempDataCritSec;
  //
  // converters
  //
  static uint32_t findSubpassCount(const RenderPassDesc &rp_desc);
  static void fillSubpassDeps(const RenderPassDesc &rp_desc, RenderPassConvertTempData &temp);
  static void fillSubpassDescs(const RenderPassDesc &rp_desc, RenderPassConvertTempData &temp);
  static void fillAttachmentDescription(const RenderPassDesc &rp_desc, RenderPassConvertTempData &temp);
  static void fillVkDependencyFromDriver(const RenderPassBind &next_bind, const RenderPassBind &prev_bind, VkSubpassDependency &dst,
    const RenderPassDesc &rp_desc, bool ro_ds_input_attachment);
  static void addStoreDependencyFromOverallAttachmentUsage(SubpassDep &dep, const RenderPassDesc &rp_desc, int32_t target);
  static void convertAttachmentRefToVersion2(VkAttachmentReference2 &dst, const VkAttachmentReference &src);
  static VkResult convertAndCreateRenderPass2(VkRenderPass *dst, const VkRenderPassCreateInfo &src,
    const Tab<VkAttachmentReference> &src_refs, const Tab<SubpassExtensions> &subpass_extensions);

  //
  // desc stores
  //

  // we need to know some target creation flags for FB bindings
  void storeTargetFormats();
  // info to keep image states consistent to current RP subpass
  void storeImageStates(const Tab<VkSubpassDescription> &subpasses, const Tab<VkAttachmentDescription> &attachments);
  // subpass summary attachment info needed for pipeline creations
  void storeSubpassAttachmentInfos();
  // input attachment indexing for auto binding on subpass changes
  void storeInputAttachments(const Tab<VkSubpassDescription> &subpasses);
  // original dependency data for self dependency barriers
  void storeSelfDeps(const Tab<VkSubpassDependency> &deps);
  // full desc hash for pipeline variations
  void storeHash();

  //
  // checkers
  //
  bool verifyMSAAResolveUsage(const RenderPassDesc &rp_desc, RenderPassConvertTempData &temp);

  // link to external states, to avoid copy and support possible MT buildup
  static const FrontRenderPassStateStorage *state;
  static BakedAttachmentSharedData *bakedAttachments;

  uint32_t activeSubpass;
  void advanceSubpass(ExecutionContext &ctx);

  void updateImageStatesForCurrentSubpass(ExecutionContext &ctx);

  void performSelfDepsForSubpass(uint32_t subpass);

  VulkanFramebufferHandle compileFB();
  VulkanFramebufferHandle compileOrGetFB();
  VkRect2D toVkArea(RenderPassArea area);
  VkExtent2D toVkFbExtent(RenderPassArea area);
  void dumpCreateInfo(VkRenderPassCreateInfo &rpci);

  struct FbWithCreationInfo
  {
    struct CompressedAtt
    {
      union
      {
        Image *img;
        struct
        {
          VkImageCreateFlags flags;
          VkImageUsageFlags usage;
          uint16_t width;
          uint16_t height;
          uint16_t layerCount;
          FormatStore fmt;
        } imageless;
      };
      uint32_t mipLayer;
      CompressedAtt() : imageless()
      {
        mipLayer = 0;
        img = nullptr;
      }
    };
    CompressedAtt atts[MAX_RENDER_PASS_ATTACHMENTS];
    VkExtent2D extent;
    VulkanFramebufferHandle handle;
  };

  Tab<FbWithCreationInfo> compiledFBs;

  std::atomic<size_t> pipelineCompileRefs = 0;

public:
  RenderPassResource(const Description &in_desc, bool manage = true);

  uint32_t getTargetsCount();
  uint32_t getCurrentSubpass();
  uint64_t getHash() { return desc.hash; }
  bool isNoOpWithoutDraws() { return desc.noOpWithoutDraws; }
  bool hasDepthAtSubpass(uint32_t subpass);
  bool isDepthAtSubpassRO(uint32_t subpass);
  bool hasDepthAtCurrentSubpass();
  bool isCurrentDepthRO();
  Image *getCurrentDepth();
  bool isCurrentDepthROAttachment(const Image *img, ImageViewState ivs) const;
  uint32_t getColorWriteMaskAtSubpass(uint32_t subpass);
  VkSampleCountFlagBits getMSAASamples(uint32_t subpass);

  void useWithState(const FrontRenderPassStateStorage &v);
  void useWithAttachments(BakedAttachmentSharedData *v);
  bool allowSRGBWrite(uint32_t index);
  void destroyFBsWithImage(const Image *img);

  void beginPass(ExecutionContext &ctx);
  void nextSubpass(ExecutionContext &ctx);
  void endPass(ExecutionContext &ctx);

  VkExtent2D getMaxActiveAreaExtent();

  void bindInputAttachments(ExecutionContext &ctx, PipelineStageStateBase &tgt, uint32_t input_index, uint32_t register_index,
    const VariatedGraphicsPipeline *pipeline);

  void addPipelineCompileRef() { ++pipelineCompileRefs; }
  void releasePipelineCompileRef() { --pipelineCompileRefs; }
  bool isPipelineCompileReferenced() { return pipelineCompileRefs.load() != 0; }
};

} // namespace drv3d_vulkan
