#include "device.h"
#include "render_pass_resource.h"
#include <perfMon/dag_statDrv.h>
#include <util/dag_hash.h>

using namespace drv3d_vulkan;

// conversion of engine RP desc to vk RP desc

void RenderPassResource::fillSubpassDeps(const RenderPassDesc &rp_desc, Tab<VkSubpassDependency> &deps)
{
  int32_t maxSubpass = 0;
  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
    maxSubpass = max(rp_desc.binds[i].subpass, maxSubpass);
  // find & fill
  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    const RenderPassBind &bind = rp_desc.binds[i];

    // keep action does no RW to target, so it should not generate dependencies
    if (bind.action & RenderPassTargetAction::RP_TA_SUBPASS_KEEP)
      continue;

    VkSubpassDependency dep{};
    dep.dstSubpass = bind.subpass == RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END ? VK_SUBPASS_EXTERNAL : bind.subpass;
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    uint32_t lastBind = -1;
    bool roDSinputAttachment = false;
    // must find in what pass we did modified target that will be used,
    // for proper dependency defenition
    if (bind.subpass != 0)
    {
      int32_t lastSubpass = RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END;
      for (uint32_t j = 0; j < rp_desc.bindCount; ++j)
      {
        if (i == j)
          continue;

        const RenderPassBind &iter = rp_desc.binds[j];
        if (iter.target != bind.target)
          continue;

        // keep action does no RW to target, so it should not generate dependencies
        if (iter.action & RenderPassTargetAction::RP_TA_SUBPASS_KEEP)
          continue;

        if (iter.subpass == bind.subpass)
        {
          bool anyDS = (iter.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL) ||
                       (bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL);
          bool bothRO = ((iter.action & bind.action) & RP_TA_SUBPASS_READ) != 0;
          roDSinputAttachment = anyDS & bothRO;
          G_ASSERTF(roDSinputAttachment,
            "vulkan: render pass <%s> desc contains multiple binds(%d, %d) for same "
            "target(%d) in same "
            "subpass(%d) with conflicting usage semantics",
            rp_desc.debugName, i, j, iter.target, iter.subpass);
          continue;
        }

        if ((iter.subpass > bind.subpass) && (bind.subpass != RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END))
          continue;

        if (iter.subpass == lastSubpass && lastSubpass > 0)
        {
          // wrong multibind checked for subpass where they happen, see roDSinputAttachment write above
          // so just set this flag if multibinds found for last ref
          roDSinputAttachment = true;
          // prioritize last ref for DS attachment, to avoid desc bind array position dependency
          if (iter.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
            lastBind = j;
        }

        if (iter.subpass > lastSubpass)
        {
          lastBind = j;
          lastSubpass = iter.subpass;
        }
      }

      // if we did found previous pass, use it, otherwise external will be left out
      if (lastSubpass >= 0)
        dep.srcSubpass = lastSubpass;
    }
    const RenderPassBind &lastBindRef = lastBind != -1 ? rp_desc.binds[lastBind] : bind;
    // debug("vulkan: prev dep for bind %u subpass %u (slot %u tgt %u) is bind %u subpass %u (slot %u tgt %u)",
    //   i, bind.subpass, bind.slot, bind.target, lastBind, lastBindRef.subpass, lastBindRef.slot, lastBindRef.target);
    if (dep.dstSubpass == VK_SUBPASS_EXTERNAL)
    {
      bool willDoInternalWriteAtPassEnd = false;
      bool willDoReadAtLastRef = false;

      // using no store, but no store is not available
      willDoInternalWriteAtPassEnd |= (bind.action & RP_TA_STORE_NONE) && !get_device().hasAttachmentNoStoreOp();
      // using store that do writes
      willDoInternalWriteAtPassEnd |= (bind.action & (RP_TA_STORE_WRITE | RP_TA_STORE_NO_CARE)) > 0;

      willDoReadAtLastRef |= (lastBindRef.action & RP_TA_SUBPASS_READ) > 0;
      // should be OK to sync with dependency when we are not at final pass
      bool isLastPass = lastBindRef.subpass == maxSubpass;

      bool needSyncBetweenReadsAndStores = willDoInternalWriteAtPassEnd & willDoReadAtLastRef & isLastPass;

      // TODO: implement layout transition at pass end
      // this will break state tracking, so avoid this for now
      // bool willDoLayoutTransition = false;
      //
      // bool toRORT = (bind.dependencyBarrier & (RB_RW_RENDER_TARGET | RB_RO_SRV)) == (RB_RW_RENDER_TARGET | RB_RO_SRV);// & isDS;
      // bool toRWRT = (bind.dependencyBarrier & RB_RW_RENDER_TARGET) && !toRORT;
      // bool toUAV = bind.dependencyBarrier & RB_RW_UAV;
      // bool toSRV = !toUAV && !toRWRT && !toRORT;
      //
      // willDoLayoutTransition |= (lastBindRef.action & RP_TA_SUBPASS_WRITE) && (toSRV | toRORT);
      // willDoLayoutTransition |= (lastBindRef.action & RP_TA_SUBPASS_READ) && toRWRT;
      // willDoLayoutTransition |= toUAV;

      bool shouldUseSelfDep = needSyncBetweenReadsAndStores; // | willDoLayoutTransition;
      if (shouldUseSelfDep)
      {
        VkSubpassDependency selfDepVk{};
        SubpassDep selfDep(selfDepVk);
        selfDep.selfDep(lastBindRef.subpass);

        bool isDS = lastBindRef.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;
        // if (needSyncBetweenReadsAndStores)
        {
          // debug("vulkan: self dep for subpass %u on reads-to-store-writes for target %u",
          //   lastBindRef.subpass, bind.target);
          selfDep.addSrc(isDS ? SubpassDep::depthShaderR() : SubpassDep::colorShaderR());
          selfDep.addDst(isDS ? SubpassDep::depthW() : SubpassDep::colorW());
        }

        // TODO: implement layout transition at pass end
        // proper self dep barrier with layout transition is needed
        // as transitions happens after stores but before external dep
        // if (willDoLayoutTransition)
        //{
        //   selfDep.addDst(isDS ? SubpassDep::depthR() : SubpassDep::colorR());
        //   selfDep.addSrc(isDS ? SubpassDep::depthR() : SubpassDep::colorR());
        //
        //   selfDep.addDst(isDS ? SubpassDep::depthW() : SubpassDep::colorW());
        //   selfDep.addSrc(isDS ? SubpassDep::depthW() : SubpassDep::colorW());
        // }

        deps.push_back(selfDepVk);
      }
    }
    // handle external deps, externally
    if ((dep.dstSubpass != VK_SUBPASS_EXTERNAL) && (dep.srcSubpass != VK_SUBPASS_EXTERNAL))
    {
      fillVkDependencyFromDriver(bind, lastBindRef, dep, rp_desc, roDSinputAttachment);
      deps.push_back(dep);
    }
  }

  // merge & compact

  for (uint32_t i = 0; i < deps.size(); ++i)
  {
    VkSubpassDependency &ldep = deps[i];
    if (ldep.srcAccessMask == 0)
      continue;

    for (uint32_t j = i + 1; j < deps.size(); ++j)
    {
      VkSubpassDependency &rdep = deps[j];
      if (rdep.srcAccessMask == 0)
        continue;

      if (
        (ldep.srcSubpass != rdep.srcSubpass) || (ldep.dstSubpass != rdep.dstSubpass) || (ldep.dependencyFlags != rdep.dependencyFlags))
        continue;

      ldep.dstAccessMask |= rdep.dstAccessMask;
      ldep.srcAccessMask |= rdep.srcAccessMask;
      ldep.dstStageMask |= rdep.dstStageMask;
      ldep.srcStageMask |= rdep.srcStageMask;

      rdep.srcAccessMask = 0;
    }
  }

  Tab<VkSubpassDependency> depsR;
  for (uint32_t i = 0; i < deps.size(); ++i)
  {
    VkSubpassDependency &ldep = deps[i];
    if (ldep.srcAccessMask == 0)
      continue;

    // debug("vulkan: dep [%u] %u -> %u", i, ldep.srcSubpass, ldep.dstSubpass);

    depsR.push_back(ldep);
  }

  deps = depsR;
}

void RenderPassResource::addStoreDependencyFromOverallAttachmentUsage(SubpassDep &dep, const RenderPassDesc &rp_desc, int32_t target)
{
  // somehow external deps need depth or color barriers if target was used once in depth/color state
  // this looks like a very strange thing, maybe validator is buggy or other deps are wrong some way

  bool usedAsDS = false;
  bool usedAsColor = false;
  for (uint32_t j = 0; j < rp_desc.bindCount; ++j)
  {
    // filter ext dep subpass & other targets
    if (rp_desc.binds[j].target != target)
      continue;
    if (rp_desc.binds[j].subpass == RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END)
      continue;

    // resolve happens at "color" stage as per spec
    if (rp_desc.binds[j].action & RP_TA_SUBPASS_RESOLVE)
      usedAsColor |= true;
    // detect DS by slot & color by any but subpass keep action
    else if (rp_desc.binds[j].slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
      usedAsDS |= true;
    else if (!(rp_desc.binds[j].action & RP_TA_SUBPASS_KEEP))
      usedAsColor |= true;

    // no need to continue if both flags are already set
    if (usedAsDS && usedAsColor)
      break;
  }

  if (usedAsColor)
  {
    dep.addPaired(SubpassDep::colorR());
    dep.addPaired(SubpassDep::colorW());
    dep.addPaired(SubpassDep::colorShaderR());
  }

  if (usedAsDS)
  {
    dep.addPaired(SubpassDep::depthR());
    dep.addPaired(SubpassDep::depthW());
    dep.addPaired(SubpassDep::depthShaderR());
  }
}

void RenderPassResource::fillVkDependencyFromDriver(const RenderPassBind &next_bind, const RenderPassBind &prev_bind,
  VkSubpassDependency &dst, const RenderPassDesc &, bool ro_ds_input_attachment)
{
  // ignore dependencyBarrier info, because it is not precise enough for VK (no precise src info)
  // ro_ds_input_attachment - use input attachment read sync instead of shader read sync

  SubpassDep dep(dst);
  bool prevDS = prev_bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;
  bool nextDS = next_bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;

  if (prev_bind.action & RP_TA_SUBPASS_READ)
    dep.addSrc((prevDS && !ro_ds_input_attachment) ? SubpassDep::depthShaderR() : SubpassDep::colorShaderR());
  else if (prev_bind.action & RP_TA_SUBPASS_WRITE)
  {
    dep.addSrc(prevDS ? SubpassDep::depthW() : SubpassDep::colorW());
    // due to OM/DS tests stages reads
    dep.addSrc(prevDS ? SubpassDep::depthR() : SubpassDep::colorR());
  }

  if (next_bind.action & RP_TA_SUBPASS_READ)
  {
    dep.addDst((nextDS && !ro_ds_input_attachment) ? SubpassDep::depthShaderR() : SubpassDep::colorShaderR());
  }
  else if (next_bind.action & RP_TA_SUBPASS_WRITE)
  {
    dep.addDst(nextDS ? SubpassDep::depthR() : SubpassDep::colorR());
    dep.addDst(nextDS ? SubpassDep::depthW() : SubpassDep::colorW());
  }
}

uint32_t RenderPassResource::findSubpassCount(const RenderPassDesc &rp_desc)
{
  int32_t subpassCount = -1;
  for (uint32_t i = 0; i < rp_desc.bindCount; ++i)
  {
    const RenderPassBind &bind = rp_desc.binds[i];
    if (bind.subpass > subpassCount)
      subpassCount = bind.subpass;
  }
  ++subpassCount;
  G_ASSERTF(subpassCount > 0, "vulkan: no subpasses in render pass <%s> description", rp_desc.debugName);

  return subpassCount;
}

void RenderPassResource::fillSubpassDescs(const RenderPassDesc &rp_desc, Tab<VkAttachmentReference> &target_refs,
  Tab<uint32_t> &target_preserves, Tab<VkSubpassDescription> &descs, Tab<SubpassExtensions> &subpass_extensions)
{
  // find subpass count
  uint32_t subpasses = findSubpassCount(rp_desc);

  struct AttCacheEl
  {
    uint8_t type;
    uint32_t idx;
  };

  Tab<AttCacheEl> attCache;
  Tab<uint32_t> refBaseForSubpass;
  Tab<uint32_t> preservesBaseForSubpass;

  descs.reserve(subpasses);
  attCache.reserve(rp_desc.targetCount);
  refBaseForSubpass.reserve(subpasses);
  target_preserves.reserve(subpasses);
  subpass_extensions.resize(subpasses);

  enum
  {
    ATT_KEEP,
    ATT_DS,
    ATT_COLOR,
    ATT_RESOLVE,
    ATT_INPUT,
    ATT_DS_RESOLVE
  };

  for (uint32_t i = 0; i < subpasses; ++i)
  {
    clear_and_shrink(attCache);
    // target_* is flattened arrays
    refBaseForSubpass.push_back(target_refs.size());
    preservesBaseForSubpass.push_back(target_preserves.size());

    VkSubpassDescription desc{};
    desc.flags = 0;
    desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    desc.inputAttachmentCount = 0;
    desc.pInputAttachments = nullptr;

    desc.colorAttachmentCount = 0;
    desc.pColorAttachments = nullptr;
    desc.pResolveAttachments = nullptr;

    desc.pDepthStencilAttachment = nullptr;

    desc.preserveAttachmentCount = 0;
    desc.pPreserveAttachments = nullptr;

    // find all pass attachments & its types
    for (uint32_t j = 0; j < rp_desc.bindCount; ++j)
    {
      const RenderPassBind &bind = rp_desc.binds[j];
      if (bind.subpass != i)
        continue;

      if (bind.action & RP_TA_SUBPASS_KEEP)
      {
        ++desc.preserveAttachmentCount;
        attCache.push_back({ATT_KEEP, j});
      }
      else if (bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
      {
        uint8_t type = ((bind.action & RP_TA_SUBPASS_RESOLVE) == RP_TA_SUBPASS_RESOLVE) ? ATT_DS_RESOLVE : ATT_DS;
        attCache.push_back({type, j});
        target_refs.push_back({});
      }
      else
      {
        if (bind.action & RP_TA_SUBPASS_RESOLVE)
          // encode with slot, as resolves must be same size as color atts
          attCache.push_back({ATT_RESOLVE, j});
        else
        {
          if (bind.action & RP_TA_SUBPASS_READ)
          {
            ++desc.inputAttachmentCount;
            attCache.push_back({ATT_INPUT, j});
            target_refs.push_back({});
          }
          else
          {
            ++desc.colorAttachmentCount;
            attCache.push_back({ATT_COLOR, j});
            target_refs.push_back({});
          }
        }
      }
    }

    // fill every type

    // preserves
    for (AttCacheEl iter : attCache)
    {
      if (iter.type != ATT_KEEP)
        continue;
      const RenderPassBind &bind = rp_desc.binds[iter.idx];

      target_preserves.push_back(bind.target);
    }

    // colors
    for (AttCacheEl iter : attCache)
    {
      if (iter.type != ATT_COLOR)
        continue;
      const RenderPassBind &bind = rp_desc.binds[iter.idx];

      G_ASSERTF(bind.slot < desc.colorAttachmentCount,
        "vulkan: RP desc should not have gaps in attachment lists"
        "<%s> uses color attachment %u when total count is %u for subpass %u",
        rp_desc.debugName, bind.slot, desc.colorAttachmentCount, i);

      VkAttachmentReference &ref = target_refs[refBaseForSubpass[i] + bind.slot];

      ref.attachment = bind.target;
      ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // inputs
    for (AttCacheEl iter : attCache)
    {
      if (iter.type != ATT_INPUT)
        continue;
      const RenderPassBind &bind = rp_desc.binds[iter.idx];

      G_ASSERTF(bind.slot < desc.inputAttachmentCount,
        "vulkan: RP desc should not have gaps in attachment lists"
        "<%s> uses input attachment %u when total count is %u for subpass %u",
        rp_desc.debugName, bind.slot, desc.inputAttachmentCount, i);

      VkAttachmentReference &ref = target_refs[refBaseForSubpass[i] + bind.slot + desc.colorAttachmentCount];

      ref.attachment = bind.target;
      ref.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // resolve
    for (AttCacheEl iter : attCache)
    {
      if (iter.type != ATT_RESOLVE)
        continue;

      for (uint32_t j = 0; j < desc.colorAttachmentCount; ++j)
        target_refs.push_back({VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

      // hint as we don't have count field
      desc.pResolveAttachments = target_refs.data();
      break;
    }

    if (desc.pResolveAttachments)
      for (AttCacheEl iter : attCache)
      {
        if (iter.type != ATT_RESOLVE)
          continue;

        const RenderPassBind &bind = rp_desc.binds[iter.idx];
        VkAttachmentReference &ref =
          target_refs[refBaseForSubpass[i] + bind.slot + desc.colorAttachmentCount + desc.inputAttachmentCount];

        ref.attachment = bind.target;
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      }

    // ds
    for (AttCacheEl iter : attCache)
    {
      if (iter.type != ATT_DS)
        continue;
      const RenderPassBind &bind = rp_desc.binds[iter.idx];
      VkAttachmentReference &ref =
        target_refs[refBaseForSubpass[i] + desc.colorAttachmentCount * (1 + (desc.pResolveAttachments ? 1 : 0)) +
                    desc.inputAttachmentCount];

      ref.attachment = bind.target;
      if (bind.action & RP_TA_SUBPASS_WRITE)
        ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      else if (bind.action & RP_TA_SUBPASS_READ)
        ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

      desc.pDepthStencilAttachment = target_refs.data(); // any ptr to mark that we have DS attachment
    }

    subpass_extensions[i].depthStencilResolveAttachmentIdx = -1;
    for (AttCacheEl iter : attCache)
    {
      if (iter.type != ATT_DS_RESOLVE)
        continue;
      const RenderPassBind &bind = rp_desc.binds[iter.idx];
      const uint32_t refIdx = refBaseForSubpass[i] + desc.colorAttachmentCount * (1 + (desc.pResolveAttachments ? 1 : 0)) +
                              desc.inputAttachmentCount + (desc.pDepthStencilAttachment ? 1 : 0);
      VkAttachmentReference &ref = target_refs[refIdx];
      ref.attachment = bind.target;
      ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      subpass_extensions[i].depthStencilResolveAttachmentIdx = int(refIdx);
      break;
    }

    descs.push_back(desc);
  }

  // fill array pointers
  for (uint32_t i = 0; i < subpasses; ++i)
  {
    VkSubpassDescription &desc = descs[i];

    if (desc.preserveAttachmentCount)
      desc.pPreserveAttachments = &target_preserves[preservesBaseForSubpass[i]];

    VkAttachmentReference *ref = &target_refs[refBaseForSubpass[i]];

    if (desc.colorAttachmentCount)
    {
      desc.pColorAttachments = ref;
      ref += desc.colorAttachmentCount;
    }

    if (desc.inputAttachmentCount)
    {
      desc.pInputAttachments = ref;
      ref += desc.inputAttachmentCount;
    }

    if (desc.pResolveAttachments)
    {
      desc.pResolveAttachments = ref;
      ref += desc.colorAttachmentCount;
    }

    if (desc.pDepthStencilAttachment)
      desc.pDepthStencilAttachment = ref;
  }
}

static inline unsigned int getAttachmentTexcf(const RenderPassDesc &rp_desc, int target_index)
{
  const RenderPassTargetDesc &extDesc = rp_desc.targetsDesc[target_index];
  if (extDesc.templateResource)
  {
    TextureInfo ti;
    extDesc.templateResource->getinfo(ti, 0);
    return ti.cflg;
  }
  return extDesc.texcf;
}

void RenderPassResource::fillAttachmentDescription(const RenderPassDesc &rp_desc, Tab<VkAttachmentDescription> &descs)
{
  descs.reserve(rp_desc.targetCount);
  for (uint32_t i = 0; i < rp_desc.targetCount; ++i)
  {
    VkAttachmentDescription desc{};
    const RenderPassTargetDesc &extDesc = rp_desc.targetsDesc[i];
    unsigned int texCf = getAttachmentTexcf(rp_desc, i);
    if (!extDesc.templateResource)
      desc.flags = extDesc.aliased ? VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT : 0;

    desc.format = FormatStore::fromCreateFlags(texCf).asVkFormat();
    bool multisample = texCf & (TEXCF_MULTISAMPLED | TEXCF_MSAATARGET);
    desc.samples = multisample ? get_device().calcMSAAQuality() : VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

    uint32_t loadActions = 0;
    uint32_t storeActions = 0;

    uint32_t lastBind = -1;
    int32_t lastSubpass = -1;

    // detect how attachment was used in last subpass with access to attachment immediate data
    //(not load/store, but "tile" storage RW)
    RenderPassTargetAction lastSubpassWithAccessAction = RenderPassTargetAction::RP_TA_NONE;
    int32_t lastSubpassWithAccess = -1;

    for (uint32_t j = 0; j < rp_desc.bindCount; ++j)
    {
      const RenderPassBind &bind = rp_desc.binds[j];

      if (bind.target != i)
        continue;

      if (bind.subpass > lastSubpass)
      {
        lastSubpass = bind.subpass;
        lastBind = j;
      }

      if ((bind.subpass > lastSubpassWithAccess) && (bind.action & (RP_TA_SUBPASS_READ | RP_TA_SUBPASS_WRITE | RP_TA_SUBPASS_RESOLVE)))
      {
        lastSubpassWithAccess = bind.subpass;
        lastSubpassWithAccessAction = bind.action;
      }

      bool isDS = bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;

      if (bind.action & RP_TA_LOAD_MASK)
      {
        ++loadActions;
        if (bind.action & RP_TA_LOAD_CLEAR)
          desc.loadOp = desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else if (bind.action & RP_TA_LOAD_READ)
          desc.loadOp = desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        else
          desc.loadOp = desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

        if (bind.action & RP_TA_SUBPASS_WRITE)
          desc.initialLayout = isDS ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        else if (bind.action & RP_TA_SUBPASS_READ)
          desc.initialLayout = isDS ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        else if (bind.action & RP_TA_SUBPASS_RESOLVE)
          desc.initialLayout = isDS ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      }

      if (bind.action & RP_TA_STORE_MASK)
      {
        ++storeActions;
        if (bind.action & RP_TA_STORE_WRITE)
          desc.storeOp = desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        else if (bind.action & RP_TA_STORE_NO_CARE)
          desc.storeOp = desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        else
        {
          if (get_device().hasAttachmentNoStoreOp())
          {
#if VK_EXT_load_store_op_none
            desc.storeOp = desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_NONE;
#elif VK_QCOM_render_pass_store_ops
            desc.storeOp = desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_NONE_QCOM;
#else
            G_ASSERTF(0, "vulkan: no store is supported by device, but not supported by vulkan header!");
#endif
          }
          else
            desc.storeOp = desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        // TODO: implement layout transition at pass end
        // if (((bind.dependencyBarrier & (RB_RW_RENDER_TARGET | RB_RO_SRV)) == (RB_RW_RENDER_TARGET | RB_RO_SRV)) && isDS)
        //   desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        // else if (bind.dependencyBarrier & RB_RW_RENDER_TARGET)
        //   desc.finalLayout = isDS ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // else if (bind.dependencyBarrier & RB_RW_UAV)
        //   desc.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
        ////transit to SRV by default if nothin usefull found
        // else// if (bind.dependencyBarrier & RB_RO_SRV)
        //   desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }

    // match layout to last used one for now (instead of dependencyBarrier)
    {
      G_ASSERTF(lastBind != -1, "vulkan: attachment %u is not used in bindings of render pass <%s>", i, rp_desc.debugName);

      const RenderPassBind &bind = rp_desc.binds[lastBind];
      bool isDS = bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;

      if (lastSubpassWithAccessAction & RP_TA_SUBPASS_WRITE)
        desc.finalLayout = isDS ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      else if (lastSubpassWithAccessAction & RP_TA_SUBPASS_READ)
        desc.finalLayout = isDS ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      else if (lastSubpassWithAccessAction & RP_TA_SUBPASS_RESOLVE)
        desc.finalLayout = isDS ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      else
        logerr("vulkan: attachment %u of render pass <%s> is unused (not used in any RW actions)", i, rp_desc.debugName);
    }

    G_ASSERTF((storeActions == 1) && (loadActions == 1),
      "vulkan: native render pass <%s> has an incorrect bind desription for target #%u:\n"
      "this target has (%u) stores and (%u) loads. It is expected to describe store and load once for every target.\n"
      "Load and Store are described via RP_TA_LOAD_*, RP_TA_STORE_*\n",
      rp_desc.debugName, i, storeActions, loadActions);

    descs.push_back(desc);
  }
}

void RenderPassResource::storeTargetCFlags()
{
  const RenderPassDesc &rpDesc = *desc.externDesc;

  clear_and_shrink(desc.targetCFlags);
  for (uint32_t i = 0; i < rpDesc.targetCount; ++i)
    desc.targetCFlags.push_back(getAttachmentTexcf(rpDesc, i));
}

void RenderPassResource::storeImageStates(const Tab<VkSubpassDescription> &subpasses, const Tab<VkAttachmentDescription> &attachments)
{
  const RenderPassDesc &rpDesc = *desc.externDesc;

  clear_and_shrink(desc.attImageLayouts);
  for (uint32_t i = 0; i < subpasses.size(); ++i)
  {
    Tab<VkImageLayout> imgStates;
    for (uint32_t j = 0; j < rpDesc.targetCount; ++j)
    {
      if (i == 0) // initial
        imgStates.push_back(attachments[j].initialLayout);
      else if (i == subpasses.size() - 1) // final
        imgStates.push_back(attachments[j].finalLayout);
      else
      {
        bool found = false;
        const VkSubpassDescription &subpass = subpasses[i];
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment == j)
        {
          found = true;
          imgStates.push_back(subpass.pDepthStencilAttachment->layout);
        }

        if (subpass.colorAttachmentCount && !found)
        {
          for (uint32_t k = 0; k < subpass.colorAttachmentCount; ++k)
          {
            if (subpass.pColorAttachments[k].attachment == j)
            {
              found = true;
              imgStates.push_back(subpass.pColorAttachments[k].layout);
              break;
            }
          }
        }

        if (subpass.inputAttachmentCount && !found)
        {
          for (uint32_t k = 0; k < subpass.inputAttachmentCount; ++k)
          {
            if (subpass.pInputAttachments[k].attachment == j)
            {
              found = true;
              imgStates.push_back(subpass.pInputAttachments[k].layout);
              break;
            }
          }
        }

        // don't care for other cases and leave in SN_RENDER_TARGET for now
        if (!found)
          imgStates.push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      }
    }
    desc.attImageLayouts.push_back(imgStates);
  }

  clear_and_shrink(desc.attImageExtrenalOperations[RenderPassDescription::EXTERNAL_OP_START]);
  clear_and_shrink(desc.attImageExtrenalOperations[RenderPassDescription::EXTERNAL_OP_END]);
  for (uint32_t i = 0; i < desc.targetCount; ++i)
  {
    {
      // external visible operation at pass end - find stage & access from overall usage
      VkSubpassDependency vkDep;
      SubpassDep dep(vkDep);
      addStoreDependencyFromOverallAttachmentUsage(dep, rpDesc, i);
      desc.attImageExtrenalOperations[RenderPassDescription::EXTERNAL_OP_END].push_back({vkDep.srcStageMask, vkDep.srcAccessMask});
    }

    {
      // external visible operation at pass start - by default find stage & access from initial layout
      //  but handle resolve differently due to them happening at color output-like stage & access regardless of depth/color targets
      bool initialUsageIsResolve = false;
      for (uint32_t j = 0; j < rpDesc.bindCount; ++j)
      {
        // filter ext dep subpass & other targets
        if (rpDesc.binds[j].target != i)
          continue;
        if (rpDesc.binds[j].subpass == RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END)
          continue;

        if (rpDesc.binds[j].action & RP_TA_LOAD_MASK)
        {
          initialUsageIsResolve = (rpDesc.binds[j].action & RP_TA_SUBPASS_RESOLVE) != 0;
          break;
        }
      }

      ExecutionSyncTracker::LogicAddress initialLaddr;
      if (initialUsageIsResolve)
        initialLaddr = ExecutionSyncTracker::LogicAddress::forAttachmentWithLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      else
        initialLaddr = ExecutionSyncTracker::LogicAddress::forAttachmentWithLayout(desc.attImageLayouts[0][i]);

      desc.attImageExtrenalOperations[RenderPassDescription::EXTERNAL_OP_START].push_back({initialLaddr.stage, initialLaddr.access});
    }
  }
}

void RenderPassResource::storeSubpassAttachmentInfos()
{
  const RenderPassDesc &rpDesc = *desc.externDesc;

  clear_and_shrink(desc.subpassAttachmentsInfos);
  for (uint32_t i = 0; i < desc.subpasses; ++i)
  {
    bool hasDepthInSubpass = false;
    uint32_t colorWriteMask = 0;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    bool hasSampleCountOneNonResolveAttachment = false;
    bool hasMultisampledAttachment = false;
    bool emittedSubpassMSAAMixError = false;
    for (uint32_t j = 0; j < rpDesc.bindCount; ++j)
    {
      const RenderPassBind &bind = rpDesc.binds[j];
      if (bind.subpass != i)
        continue;

      bool isDS = (bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL);
      hasDepthInSubpass |= isDS;

      if (!isDS && (bind.action & RP_TA_SUBPASS_WRITE))
        colorWriteMask |= 1 << bind.slot;

      unsigned texcf = getAttachmentTexcf(rpDesc, bind.target);
      const bool hasMultisampleFlags = texcf & (TEXCF_MULTISAMPLED | TEXCF_MSAATARGET);
      VkSampleCountFlagBits samples =
        hasMultisampleFlags ? get_device().calcMSAAQuality() : VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
      msaaSamples = eastl::max(msaaSamples, samples);
      const bool isResolve = bind.action & RP_TA_SUBPASS_RESOLVE;
      const bool isMultisampled = samples != VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

      if (!isMultisampled && !isResolve)
      {
        hasSampleCountOneNonResolveAttachment = true;
      }
      if (isMultisampled)
      {
        hasMultisampledAttachment = true;
      }
      if (hasMultisampleFlags && !isMultisampled)
      {
        logwarn("vulkan: <%s> has target with MSAA flags while MSAA is disabled at bind: %u target: %u", rpDesc.debugName, j,
          bind.target);
      }
      if (isResolve && isMultisampled)
      {
        logerr("vulkan: <%s> has multisampled resolve attachment at bind: %u target: %u", rpDesc.debugName, j, bind.target);
      }
      if (hasMultisampledAttachment && hasSampleCountOneNonResolveAttachment && !emittedSubpassMSAAMixError)
      {
        logerr("vulkan: <%s> has a mix of multisampled and one-sampled non-resolve attachments in subpass: %u", rpDesc.debugName, i);
        emittedSubpassMSAAMixError = true;
      }
    }
    desc.subpassAttachmentsInfos.push_back({hasDepthInSubpass, colorWriteMask, msaaSamples});
  }
}

void RenderPassResource::storeInputAttachments(const Tab<VkSubpassDescription> &subpasses)
{
  clear_and_shrink(desc.inputAttachments);
  for (uint32_t i = 0; i < subpasses.size(); ++i)
  {
    Tab<uint32_t> inputAttachments;
    for (uint32_t j = 0; j < subpasses[i].inputAttachmentCount; ++j)
      inputAttachments.push_back(subpasses[i].pInputAttachments[j].attachment);
    desc.inputAttachments.push_back(inputAttachments);
  }
}

void RenderPassResource::storeSelfDeps(const Tab<VkSubpassDependency> &deps)
{
  for (uint32_t i = 0; i < desc.subpasses; ++i)
  {
    VkSubpassDependency emptyDep;
    emptyDep.dependencyFlags = 0;
    desc.selfDeps.push_back(emptyDep);
  }

  for (uint32_t i = 0; i < deps.size(); ++i)
  {
    if (deps[i].dependencyFlags & VK_DEPENDENCY_BY_REGION_BIT)
      desc.selfDeps[deps[i].srcSubpass] = deps[i];
  }
}
template <typename T>
void fnv1_helper(T *data, uint64_t &hash)
{
  uint8_t *dt = (uint8_t *)data;
  for (int i = 0; i < sizeof(T); ++i)
    hash = fnv1a_step<64>(dt[i], hash);
}

void RenderPassResource::storeHash()
{
  // TODO: check hash collision
  const RenderPassDesc &rpDesc = *desc.externDesc;

  uint64_t ret = FNV1Params<64>::offset_basis;

  G_ASSERTF(rpDesc.targetCount <= 0xFF, "vulkan: target count %u are over limit (%u) for hashing for render pass <%s>",
    rpDesc.targetCount, 0xFF, rpDesc.debugName);
  G_ASSERTF(rpDesc.bindCount <= 0xFF, "vulkan: target count %u are over limit (%u) for hashing for render pass <%s>", rpDesc.bindCount,
    0xFF, rpDesc.debugName);

  ret = fnv1a_step<64>(rpDesc.targetCount, ret);
  ret = fnv1a_step<64>(rpDesc.bindCount, ret);

  for (uint32_t i = 0; i < rpDesc.targetCount; ++i)
  {
    unsigned int texCf = getAttachmentTexcf(rpDesc, i);
    fnv1_helper(&texCf, ret);
    const RenderPassTargetDesc &tgt = rpDesc.targetsDesc[i];
    ret = fnv1a_step<64>(tgt.templateResource ? 0 : (uint8_t)tgt.aliased, ret);
  }

  for (uint32_t i = 0; i < rpDesc.bindCount; ++i)
  {
    const RenderPassBind &bind = rpDesc.binds[i];
    ret = fnv1a_step<64>(bind.subpass, ret);
    ret = fnv1a_step<64>(bind.target, ret);
    ret = fnv1a_step<64>(bind.slot, ret);
    fnv1_helper(&bind.dependencyBarrier, ret);
    fnv1_helper(&bind.action, ret);
  }

  desc.hash = ret;
}

void RenderPassResource::dumpCreateInfo(VkRenderPassCreateInfo &rpci)
{
  debug("vulkan: native rp: %p : %s", getBaseHandle(), desc.externDesc->debugName);
  debug("vulkan: native rp: attachmentCount: %u", rpci.attachmentCount);
  debug("vulkan: native rp: dependencyCount: %u", rpci.dependencyCount);
  debug("vulkan: native rp: subpassCount: %u", rpci.subpassCount);

  for (uint32_t i = 0; i < rpci.attachmentCount; ++i)
  {
    debug("vulkan: native rp: attachment %u: flags: %X", i, rpci.pAttachments[i].flags);
    debug("vulkan: native rp: attachment %u: format: %s", i, FormatStore::fromVkFormat(rpci.pAttachments[i].format).getNameString());
    debug("vulkan: native rp: attachment %u: samples: %X", i, rpci.pAttachments[i].samples);
    debug("vulkan: native rp: attachment %u: loadOp: %s", i, formatAttachmentLoadOp(rpci.pAttachments[i].loadOp));
    debug("vulkan: native rp: attachment %u: storeOp: %s", i, formatAttachmentStoreOp(rpci.pAttachments[i].storeOp));
    debug("vulkan: native rp: attachment %u: stencilLoadOp: %s", i, formatAttachmentLoadOp(rpci.pAttachments[i].stencilLoadOp));
    debug("vulkan: native rp: attachment %u: stencilStoreOp: %s", i, formatAttachmentStoreOp(rpci.pAttachments[i].stencilStoreOp));
    debug("vulkan: native rp: attachment %u: initialLayout: %s", i, formatImageLayout(rpci.pAttachments[i].initialLayout));
    debug("vulkan: native rp: attachment %u: finalLayout: %s", i, formatImageLayout(rpci.pAttachments[i].finalLayout));
  }
  for (uint32_t i = 0; i < rpci.dependencyCount; ++i)
  {
    debug("vulkan: native rp: dependency: %u: srcSubpass: %u", i, rpci.pDependencies[i].srcSubpass);
    debug("vulkan: native rp: dependency: %u: dstSubpass: %u", i, rpci.pDependencies[i].dstSubpass);
    debug("vulkan: native rp: dependency: %u: srcStageMask: %s", i, formatPipelineStageFlags(rpci.pDependencies[i].srcStageMask));
    debug("vulkan: native rp: dependency: %u: dstStageMask: %s", i, formatPipelineStageFlags(rpci.pDependencies[i].dstStageMask));
    debug("vulkan: native rp: dependency: %u: srcAccessMask: %s", i, formatMemoryAccessFlags(rpci.pDependencies[i].srcAccessMask));
    debug("vulkan: native rp: dependency: %u: dstAccessMask: %s", i, formatMemoryAccessFlags(rpci.pDependencies[i].dstAccessMask));
    debug("vulkan: native rp: dependency: %u: dependencyFlags: %u", i, rpci.pDependencies[i].dependencyFlags);
  }
  for (uint32_t i = 0; i < rpci.subpassCount; ++i)
  {
    debug("vulkan: native rp: subpass %u: flags: %X", i, rpci.pSubpasses[i].flags);
    debug("vulkan: native rp: subpass %u: pipelineBindPoint: %X", i, rpci.pSubpasses[i].pipelineBindPoint);
    debug("vulkan: native rp: subpass %u: inputAttachmentCount: %u", i, rpci.pSubpasses[i].inputAttachmentCount);
    debug("vulkan: native rp: subpass %u: colorAttachmentCount: %u", i, rpci.pSubpasses[i].colorAttachmentCount);
    debug("vulkan: native rp: subpass %u: preserveAttachmentCount: %u", i, rpci.pSubpasses[i].preserveAttachmentCount);
    for (uint32_t j = 0; j < rpci.pSubpasses[i].inputAttachmentCount; ++j)
    {
      debug("vulkan: native rp: subpass %u: inputAttachment: %u: idx %u", i, j, rpci.pSubpasses[i].pInputAttachments[j].attachment);
      debug("vulkan: native rp: subpass %u: inputAttachment: %u: layout %s", i, j,
        formatImageLayout(rpci.pSubpasses[i].pInputAttachments[j].layout));
    }
    for (uint32_t j = 0; j < rpci.pSubpasses[i].colorAttachmentCount; ++j)
    {
      debug("vulkan: native rp: subpass %u: colorAttachment: %u: idx %u", i, j, rpci.pSubpasses[i].pColorAttachments[j].attachment);
      debug("vulkan: native rp: subpass %u: colorAttachment: %u: layout %s", i, j,
        formatImageLayout(rpci.pSubpasses[i].pColorAttachments[j].layout));
    }
    if (rpci.pSubpasses[i].pResolveAttachments)
      for (uint32_t j = 0; j < rpci.pSubpasses[i].colorAttachmentCount; ++j)
      {
        debug("vulkan: native rp: subpass %u: resolveAttachment: %u: idx %u", i, j,
          rpci.pSubpasses[i].pResolveAttachments[j].attachment);
        debug("vulkan: native rp: subpass %u: resolveAttachment: %u: layout %s", i, j,
          formatImageLayout(rpci.pSubpasses[i].pResolveAttachments[j].layout));
      }

    if (rpci.pSubpasses[i].pDepthStencilAttachment)
    {
      debug("vulkan: native rp: subpass %u: depthAttachment: idx %u", i, rpci.pSubpasses[i].pDepthStencilAttachment[0].attachment);
      debug("vulkan: native rp: subpass %u: depthAttachment: layout %s", i,
        formatImageLayout(rpci.pSubpasses[i].pDepthStencilAttachment[0].layout));
    }

    for (uint32_t j = 0; j < rpci.pSubpasses[i].preserveAttachmentCount; ++j)
      debug("vulkan: native rp: subpass %u: preserveAttachment: %u: idx %u", i, j, rpci.pSubpasses[i].pPreserveAttachments[j]);
  }
}

#define G_MIGRATE_PARAM(dstName, srcName, paramName) dstName.paramName = srcName.paramName

void RenderPassResource::convertAttachmentRefToVersion2(VkAttachmentReference2 &dst, const VkAttachmentReference &src)
{
  dst.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
  dst.pNext = nullptr;
  G_MIGRATE_PARAM(dst, src, attachment);
  G_MIGRATE_PARAM(dst, src, layout);
  dst.aspectMask = 0; // May be wrong
}

VkResult RenderPassResource::convertAndCreateRenderPass2(VkRenderPass *dst, const VkRenderPassCreateInfo &src,
  const Tab<VkAttachmentReference> &src_refs, const Tab<SubpassExtensions> &subpass_extensions)
{
  VkRenderPassCreateInfo2 rpci = {};
  Tab<VkSubpassDependency2> deps;
  Tab<VkSubpassDescription2> subpasses;
  Tab<VkAttachmentReference2> refs;
  Tab<VkAttachmentDescription2> attachments;
  Tab<VkSubpassDescriptionDepthStencilResolve> subpassDepthStencilResolveExts;

  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  refs.resize(src_refs.size());
  {
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
    rpci.pNext = nullptr;
    rpci.correlatedViewMaskCount = 0;
    rpci.pCorrelatedViewMasks = nullptr;
  }
  {
    rpci.attachmentCount = src.attachmentCount;
    attachments.resize(rpci.attachmentCount);
    rpci.pAttachments = attachments.data();
    for (int i = 0; i < rpci.attachmentCount; ++i)
    {
      VkAttachmentDescription2 &dstAtt = attachments[i];
      const VkAttachmentDescription &srcAtt = src.pAttachments[i];
      dstAtt.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
      dstAtt.pNext = nullptr;
      G_MIGRATE_PARAM(dstAtt, srcAtt, flags);
      G_MIGRATE_PARAM(dstAtt, srcAtt, format);
      G_MIGRATE_PARAM(dstAtt, srcAtt, samples);
      G_MIGRATE_PARAM(dstAtt, srcAtt, loadOp);
      G_MIGRATE_PARAM(dstAtt, srcAtt, storeOp);
      G_MIGRATE_PARAM(dstAtt, srcAtt, stencilLoadOp);
      G_MIGRATE_PARAM(dstAtt, srcAtt, stencilStoreOp);
      G_MIGRATE_PARAM(dstAtt, srcAtt, initialLayout);
      G_MIGRATE_PARAM(dstAtt, srcAtt, finalLayout);
    }
  }
  {
    rpci.dependencyCount = src.dependencyCount;
    deps.resize(rpci.dependencyCount);
    rpci.pDependencies = deps.data();
    for (int i = 0; i < rpci.dependencyCount; ++i)
    {
      VkSubpassDependency2 &dstDep = deps[i];
      const VkSubpassDependency &srcDep = src.pDependencies[i];
      dstDep.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
      dstDep.pNext = nullptr;
      G_MIGRATE_PARAM(dstDep, srcDep, srcSubpass);
      G_MIGRATE_PARAM(dstDep, srcDep, dstSubpass);
      G_MIGRATE_PARAM(dstDep, srcDep, srcStageMask);
      G_MIGRATE_PARAM(dstDep, srcDep, dstStageMask);
      G_MIGRATE_PARAM(dstDep, srcDep, srcAccessMask);
      G_MIGRATE_PARAM(dstDep, srcDep, dstAccessMask);
      G_MIGRATE_PARAM(dstDep, srcDep, dependencyFlags);
      dstDep.viewOffset = 0;
    }
  }
  {
    int attRefHead = 0;
    rpci.subpassCount = src.subpassCount;
    subpasses.resize(rpci.subpassCount);
    subpassDepthStencilResolveExts.resize(rpci.subpassCount);
    rpci.pSubpasses = subpasses.data();
    for (int subpassIdx = 0; subpassIdx < rpci.subpassCount; ++subpassIdx)
    {
      VkSubpassDescription2 &dstSubpass = subpasses[subpassIdx];
      const VkSubpassDescription &srcSubpass = src.pSubpasses[subpassIdx];
      const SubpassExtensions &subpassExtensions = subpass_extensions[subpassIdx];
      dstSubpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
      dstSubpass.pNext = nullptr;
      dstSubpass.viewMask = 0;
      G_MIGRATE_PARAM(dstSubpass, srcSubpass, flags);
      G_MIGRATE_PARAM(dstSubpass, srcSubpass, pipelineBindPoint);
      G_MIGRATE_PARAM(dstSubpass, srcSubpass, inputAttachmentCount);
      dstSubpass.pInputAttachments = dstSubpass.inputAttachmentCount ? (refs.data() + attRefHead) : nullptr;
      for (int i = 0; i < dstSubpass.inputAttachmentCount; ++i)
      {
        convertAttachmentRefToVersion2(refs[attRefHead], srcSubpass.pInputAttachments[i]);
        attRefHead++;
      }
      G_MIGRATE_PARAM(dstSubpass, srcSubpass, colorAttachmentCount);
      dstSubpass.pColorAttachments = dstSubpass.colorAttachmentCount ? (refs.data() + attRefHead) : nullptr;
      for (int i = 0; i < dstSubpass.colorAttachmentCount; ++i)
      {
        convertAttachmentRefToVersion2(refs[attRefHead], srcSubpass.pColorAttachments[i]);
        attRefHead++;
      }
      if (srcSubpass.pResolveAttachments)
      {
        dstSubpass.pResolveAttachments = (refs.data() + attRefHead);
        for (int i = 0; i < dstSubpass.colorAttachmentCount; ++i)
        {
          convertAttachmentRefToVersion2(refs[attRefHead], srcSubpass.pResolveAttachments[i]);
          attRefHead++;
        }
      }
      else
      {
        dstSubpass.pResolveAttachments = nullptr;
      }
      if (srcSubpass.pDepthStencilAttachment)
      {
        convertAttachmentRefToVersion2(refs[attRefHead], *srcSubpass.pDepthStencilAttachment);
        dstSubpass.pDepthStencilAttachment = (refs.data() + attRefHead);
        attRefHead++;
      }
      else
      {
        dstSubpass.pDepthStencilAttachment = nullptr;
      }
      G_MIGRATE_PARAM(dstSubpass, srcSubpass, preserveAttachmentCount);
      G_MIGRATE_PARAM(dstSubpass, srcSubpass, pPreserveAttachments); // Copying only ptr!
      if (subpassExtensions.depthStencilResolveAttachmentIdx >= 0)
      {
        if (subpassExtensions.depthStencilResolveAttachmentIdx >= src_refs.size())
        {
          fatal("vulkan: wrong depthStencilResolveAttachmentIdx: %d", subpassExtensions.depthStencilResolveAttachmentIdx);
        }
        if (!drvDev.hasDepthStencilResolve())
        {
          fatal("vulkan: depth_stencil_resolve is requested, but not supported");
        }
#if VK_KHR_depth_stencil_resolve
        convertAttachmentRefToVersion2(refs[attRefHead], src_refs[subpassExtensions.depthStencilResolveAttachmentIdx]);

        dstSubpass.pNext = &subpassDepthStencilResolveExts[subpassIdx];
        VkSubpassDescriptionDepthStencilResolve &depthStencilResolve = subpassDepthStencilResolveExts[subpassIdx];
        depthStencilResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
        depthStencilResolve.pNext = nullptr;
        depthStencilResolve.pDepthStencilResolveAttachment = (refs.data() + attRefHead);

        const VkPhysicalDeviceDepthStencilResolveProperties &dsrProps = drvDev.getDeviceProperties().depthStencilResolveProps;
        // According to the spec, only VK_RESOLVE_MODE_SAMPLE_ZERO_BIT is guaranteed to be available
        // We are trying to use VK_RESOLVE_MODE_MIN_BIT to use the most far depth when possible
        // so that pixels with both far and close subpixles will be overwritten by VFX
        depthStencilResolve.stencilResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        depthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        if (dsrProps.supportedDepthResolveModes & VK_RESOLVE_MODE_MIN_BIT)
        {
          if (dsrProps.independentResolveNone == VK_TRUE)
          {
            depthStencilResolve.stencilResolveMode = VK_RESOLVE_MODE_NONE;
            depthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_MIN_BIT;
          }
          else if (dsrProps.supportedStencilResolveModes & VK_RESOLVE_MODE_MIN_BIT)
          {
            depthStencilResolve.stencilResolveMode = VK_RESOLVE_MODE_MIN_BIT;
            depthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_MIN_BIT;
          }
        }
#endif
        attRefHead++;
      }
    }
    G_ASSERT(attRefHead == int(src_refs.size()));
  }
  return dev.vkCreateRenderPass2KHR(dev.get(), &rpci, nullptr, dst);
}

#undef G_MIGRATE_PARAM

void RenderPassResource::createVulkanObject()
{
  TIME_PROFILE(vulkan_native_rp_create);
  G_ASSERTF(desc.externDesc, "vulkan: should provide valid external render pass description");
  const RenderPassDesc &rpDesc = *desc.externDesc; // -V522

  G_ASSERTF(rpDesc.targetCount < MAX_RENDER_PASS_ATTACHMENTS, "vulkan: too much (%u >= %u) targets for render pass %u",
    rpDesc.targetCount, MAX_RENDER_PASS_ATTACHMENTS, rpDesc.debugName);

  Tab<VkSubpassDependency> deps;
  Tab<VkSubpassDescription> subpasses;
  Tab<VkAttachmentReference> refs;
  Tab<uint32_t> preserves;
  Tab<VkAttachmentDescription> attachments;
  Tab<SubpassExtensions> subpass_extensions;

  fillSubpassDeps(rpDesc, deps);
  fillSubpassDescs(rpDesc, refs, preserves, subpasses, subpass_extensions);
  fillAttachmentDescription(rpDesc, attachments);

  bool needToUseAPIVersion2 = false;
  for (int subpassIdx = 0; subpassIdx < subpasses.size(); ++subpassIdx)
  {
    const SubpassExtensions &subpassExtensions = subpass_extensions[subpassIdx];
    if (subpassExtensions.depthStencilResolveAttachmentIdx >= 0)
    {
      needToUseAPIVersion2 = true;
      break;
    }
  }
  {
    VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.dependencyCount = deps.size();
    rpci.pDependencies = deps.data();
    rpci.subpassCount = subpasses.size();
    rpci.pSubpasses = subpasses.data();
    rpci.attachmentCount = attachments.size();
    rpci.pAttachments = attachments.data();

    Device &drvDev = get_device();
    VulkanDevice &dev = drvDev.getVkDevice();

    Handle ret{};
    if (needToUseAPIVersion2)
    {
      if (drvDev.hasCreateRenderPass2())
      {
        VULKAN_EXIT_ON_FAIL(convertAndCreateRenderPass2(ptr(ret), rpci, refs, subpass_extensions));
      }
      else
      {
        fatal("vulkan: trying to use unavailable create_renderpass2 for <%s>", rpDesc.debugName);
      }
    }
    else
    {
      VULKAN_EXIT_ON_FAIL(dev.vkCreateRenderPass(dev.get(), &rpci, nullptr, ptr(ret)));
    }
    setHandle(generalize(ret));

    // dump create info if something is wrong about generated object
    // dumpCreateInfo(rpci);
  }

  // externDesc is no longer available outside of creation call chain
  // store stuff that still needed & cleanup pointer
  desc.subpasses = subpasses.size();
  desc.targetCount = rpDesc.targetCount;
  storeTargetCFlags();
  storeSubpassAttachmentInfos();
  storeInputAttachments(subpasses);
  storeSelfDeps(deps);
  storeImageStates(subpasses, attachments);
  storeHash();

  desc.externDesc = nullptr;
}
