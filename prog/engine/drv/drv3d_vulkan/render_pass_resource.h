#pragma once
#include "device_resource.h"
#include "subpass_dependency.h"
#include "image_resource.h"

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

  struct SubpassAttachmentsInfo
  {
    bool hasDSBind;
    uint32_t colorWriteMask;
    VkSampleCountFlagBits msaaSamples;
  };

  uint32_t targetCount;
  uint32_t subpasses;
  Tab<unsigned> targetCFlags;
  Tab<SubpassAttachmentsInfo> subpassAttachmentsInfos;
  Tab<VkSubpassDependency> selfDeps;
  Tab<Tab<uint32_t>> inputAttachments;
  Tab<Tab<VkImageLayout>> attImageLayouts;
  uint64_t hash;

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
  void bindMemory();
  void reuseHandle();
  void evict();
  void restoreFromSysCopy();
  bool isEvictable();
  void shutdown();
  bool nonResidentCreation();
  void makeSysCopy();

  template <int Tag>
  void onDelayedCleanupBackend(ContextBackend &){};

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

  enum ImageLayoutSync
  {
    START_EDGE,
    INNER,
    END_EDGE
  };

private:
  struct SubpassExtensions
  {
    // index into VkAttachmentReference array
    // -1 means that no depthStencilResolve struct should be chained
    int depthStencilResolveAttachmentIdx = -1;
  };
  //
  // converters
  //
  static uint32_t findSubpassCount(const RenderPassDesc &rp_desc);
  static void fillSubpassDeps(const RenderPassDesc &rp_desc, Tab<VkSubpassDependency> &deps);
  static void fillSubpassDescs(const RenderPassDesc &rp_desc, Tab<VkAttachmentReference> &target_refs, Tab<uint32_t> &target_preserves,
    Tab<VkSubpassDescription> &descs, Tab<SubpassExtensions> &subpass_extensions);
  static void fillAttachmentDescription(const RenderPassDesc &rp_desc, Tab<VkAttachmentDescription> &descs);
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
  void storeTargetCFlags();
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


  // link to external states, to avoid copy and support possible MT buildup
  static const FrontRenderPassStateStorage *state;
  static BakedAttachmentSharedData *bakedAttachments;

  uint32_t activeSubpass;
  void advanceSubpass(ExecutionContext &ctx);

  void setFrameImageLayout(ExecutionContext &ctx, uint32_t att_index, VkImageLayout new_state, ImageLayoutSync sync_type);
  void updateImageStatesForCurrentSubpass(ExecutionContext &ctx);

  void performSelfDepsForSubpass(uint32_t subpass, VulkanCommandBufferHandle cmd_b);

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

public:
  RenderPassResource(const Description &in_desc, bool manage = true);

  uint32_t getTargetsCount();
  uint32_t getCurrentSubpass();
  uint64_t getHash() { return desc.hash; }
  bool hasDepthAtSubpass(uint32_t subpass);
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
};

} // namespace drv3d_vulkan
