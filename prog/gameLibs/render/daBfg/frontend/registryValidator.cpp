// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "registryValidator.h"

#include "frontend/dependencyData.h"
#include "frontend/internalRegistry.h"
#include "frontend/nameResolver.h"
#include "common/genericPoint.h"

#include <memory/dag_framemem.h>
#include <dag/dag_vectorSet.h>


namespace dabfg
{

void RegistryValidator::validateLifetimes(ValidityInfo &validity) const
{
  for (auto [resId, lifetime] : depData.resourceLifetimes.enumerate())
  {
    if (registry.resources[resId].history != History::No && lifetime.consumedBy != NodeNameId::Invalid)
    {
      logerr("Resource '%s' was consumed by '%s' on it's frame but had it's "
             "history requested on next frame! "
             "This is impossible to satisfy, disabling the resource! ",
        registry.knownNames.getName(resId), registry.knownNames.getName(lifetime.consumedBy));

      validity.resourceValid.set(resId, false);
    }

    if (lifetime.introducedBy != NodeNameId::Invalid && lifetime.introducedBy == lifetime.consumedBy)
    {
      logerr("Resource '%s' was both created and consumed by node '%s'! "
             "This is impossible to satisfy, disabling the resource!",
        registry.knownNames.getName(resId), registry.knownNames.getName(lifetime.introducedBy));

      validity.resourceValid.set(resId, false);
    }

    dag::RelocatableFixedVector<NodeNameId, 32> sortedReaders;
    sortedReaders.assign(lifetime.readers.begin(), lifetime.readers.end());
    eastl::sort(sortedReaders.begin(), sortedReaders.end());

    if (eastl::binary_search(sortedReaders.begin(), sortedReaders.end(), lifetime.introducedBy))
    {
      logerr("Resource '%s' was both read and introduced by node '%s'! "
             "This is impossible to satisfy, disabling the resource! ",
        registry.knownNames.getName(resId), registry.knownNames.getName(lifetime.introducedBy));

      validity.resourceValid.set(resId, false);
    }

    if (eastl::binary_search(sortedReaders.begin(), sortedReaders.end(), lifetime.consumedBy))
    {
      logerr("Resource '%s' was both read and consumed by node '%s'! "
             "This is impossible to satisfy, disabling the node! ",
        registry.knownNames.getName(resId), registry.knownNames.getName(lifetime.consumedBy));

      validity.nodeValid.set(lifetime.consumedBy, false);
    }

    dag::RelocatableFixedVector<NodeNameId, 32> sortedModifiers;
    sortedModifiers.assign(lifetime.modificationChain.begin(), lifetime.modificationChain.end());
    eastl::sort(sortedModifiers.begin(), sortedModifiers.end());

    dag::RelocatableFixedVector<NodeNameId, 8> conflicts;
    eastl::set_intersection(sortedReaders.begin(), sortedReaders.end(), sortedModifiers.begin(), sortedModifiers.end(),
      eastl::back_insert_iterator<decltype(conflicts)>(conflicts));

    if (!conflicts.empty())
    {
      eastl::string list;
      for (const auto nodeId : conflicts)
      {
        list += "'";
        list += registry.knownNames.getName(nodeId);
        list += "' ";
      }

      logerr("Found nodes that both modify and read resource '%s'! Offender(s): %s. "
             "This is impossible to satisfy, disabling these nodes!",
        registry.knownNames.getName(resId), list.c_str());

      for (const auto nodeId : conflicts)
        validity.nodeValid.set(nodeId, false);
    }
  }
}

bool RegistryValidator::validateResource(ResNameId resId) const
{
  const auto &lifetime = depData.resourceLifetimes[resId];

  const bool wasIntroduced = lifetime.introducedBy != NodeNameId::Invalid;
  const bool isASlot = registry.resourceSlots[resId].has_value();
  if (isASlot && wasIntroduced)
    logerr("The name '%s' was both used to create/register/rename "
           "a resource in node '%s' and was filled as a resource slot. "
           "Please, use a different name for one of those!",
      registry.knownNames.getName(resId), registry.knownNames.getName(lifetime.introducedBy));

  const auto checkResolution = [this](AutoResTypeNameId unresolvedAutoResId) {
    const auto autoResId = nameResolver.resolve(unresolvedAutoResId);

    const bool positive = eastl::visit(
      [&](const auto &values) {
        if constexpr (eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
          return false;
        else
          return all_greater(values.staticResolution, 0) && all_greater(values.dynamicResolution, 0);
      },
      registry.autoResTypes[autoResId].values);

    return positive;
  };

  bool isInvalidBuffer = false;
  if (auto desc = eastl::get_if<ResourceDescription>(&registry.resources[resId].creationInfo);
      desc && desc->resType == RES3D_SBUF && desc->asBufferRes.elementCount == 0)
  {
    isInvalidBuffer = true;
    logerr("Buffer '%s' was created with element count of 0, which is not valid!", registry.knownNames.getName(resId));
  }

  const bool autoResValid =
    !registry.resources[resId].resolution.has_value() || checkResolution(registry.resources[resId].resolution->id);

  if (!autoResValid)
    logerr("Resource '%s' was created with auto-resolution '%s', "
           "but this resolution was either set to an invalid value or "
           "not set at all!",
      registry.knownNames.getName(resId), registry.knownNames.getName(registry.resources[resId].resolution->id));

  // Slots stay marked as invalid cuz they don't represent real resources.
  // Things that had a conflict are invalid.
  return wasIntroduced && !isASlot && !isInvalidBuffer && autoResValid;
}

bool RegistryValidator::validateNode(NodeNameId nodeId) const
{
  const auto &nodeData = registry.nodes[nodeId];

  bool anyUnfilledSlotRequests = false;
  for (const auto &[resId, req] : nodeData.resourceRequests)
    if (req.slotRequest && !req.optional && !registry.resourceSlots[resId].has_value())
    {
      logerr("Found a broken node '%s'! "
             "Reason: it requested the resource in slot '%s' as mandatory, "
             "but this slot was never actually filled! Either a "
             "typo in the name, or this is supposed to be a regular "
             "resource request (not a slot one), or a missing call "
             "to fill_slot! Disabling this node!",
        registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));
      anyUnfilledSlotRequests = true;
    }

  bool anyBindingTypeMismatches = false;
  for (const auto &[bindId, req] : nodeData.bindings)
  {
    // TODO: validate that subtypes match when binding matrices
    if (req.type != BindingType::ShaderVar)
      continue;

    const auto resType = resourceDataFor(req.resource).type;
    const auto svType = ShaderGlobal::get_var_type(bindId);

    const auto resIt = nodeData.resourceRequests.find(req.resource);
    if (resIt != nodeData.resourceRequests.end() && resIt->second.optional && svType == -1)
      continue;

    static constexpr eastl::array<char const *const, 10> SHVT_MESSAGES{"is missing from the shader map", "only accepts INTs",
      "only accepts REALs", "only accepts COLOR4s", "only accepts TEXTUREs", "only accepts BUFFERs", "only accepts INT4s",
      "only accepts FLOAT4X4s", "only accepts SAMPLERs", "only accepts TLASes"};

    G_ASSERT_CONTINUE(svType >= -1 && 1 + svType < SHVT_MESSAGES.size()); // Sanity check

    auto handleMismatch = [this, &anyBindingTypeMismatches, svType, nodeId = nodeId, svId = bindId, resId = req.resource](
                            const char *res_type_str) {
      logerr("Node '%s' requested %s '%s' to be set to"
             " shader variable '%s', but the variable %s!"
             " Either the programmers messed up, or the shader dump is out of date!",
        registry.knownNames.getName(nodeId), res_type_str, registry.knownNames.getName(resId), VariableMap::getVariableName(svId),
        SHVT_MESSAGES[1 + svType]);
      anyBindingTypeMismatches = false;
    };

    switch (resType)
    {
      case ResourceType::Texture:
        if (svType != SHVT_TEXTURE)
          handleMismatch("texture");
        break;

      case ResourceType::Buffer:
        if (svType != SHVT_BUFFER)
          handleMismatch("buffer");
        break;

      case ResourceType::Blob:
        // TODO: validate blob type using subtypes
        if (svType == SHVT_BUFFER || svType == SHVT_TEXTURE || svType == -1)
          handleMismatch("blob");
        break;

      // If resource is missing, either this is an optional request
      // and everything will work fine, or the unfulfilled mandatory
      // request will get caught down below.
      case ResourceType::Invalid: break;
    }
  }

  bool anyUsageConflictsAfterNameResolution = false;

  const auto checkResIdRequest = [this, nodeId, &anyUsageConflictsAfterNameResolution](bool history, auto resolvedResId,
                                   eastl::span<const ResNameId> unresolvedResIds) {
    G_ASSERT(!unresolvedResIds.empty());

    // We expect to always find a correct request, otherwise nameResolver is broken.
    const auto getUsage = [&](ResNameId unresolvedResId) -> eastl::optional<ResourceUsage> {
      const auto &requests = history ? registry.nodes[nodeId].historyResourceReadRequests : registry.nodes[nodeId].resourceRequests;

      const auto reqIt = requests.find(unresolvedResId);
      if (reqIt != requests.end())
        return reqIt->second.usage;

      return eastl::nullopt;
    };

    const auto firstUsage = getUsage(unresolvedResIds.front());
    G_ASSERT_RETURN(firstUsage.has_value(), );

    for (const auto unresolvedResId : unresolvedResIds)
    {
      const auto usage = getUsage(unresolvedResId);
      G_ASSERT_CONTINUE(usage.has_value());

      if (firstUsage->access != usage->access || firstUsage->type != usage->type)
      {
        logerr("Resource requests for names '%s' and '%s' both resolved to the same resource with name '%s' "
               "within the node '%s', but the requested usages were different! This would result in impossible "
               "barriers, disabling this node!",
          registry.knownNames.getName(unresolvedResIds.front()), registry.knownNames.getName(unresolvedResId),
          registry.knownNames.getName(resolvedResId), registry.knownNames.getName(nodeId));
        anyUsageConflictsAfterNameResolution = true;
      }
    }
  };

  nameResolver.iterateInverseMapping(nodeId, checkResIdRequest);

  const auto isBackbuffer = [this](ResNameId resId) {
    return eastl::holds_alternative<DriverDeferredTexture>(resourceDataFor(resId).creationInfo);
  };

  bool anyInvalidBackbufferRequests = false;
  for (const auto &[resId, req] : nodeData.resourceRequests)
    if (isBackbuffer(resId))
    {
      const bool usageAsExpected =
        req.usage.access == Access::READ_WRITE && req.usage.type == Usage::COLOR_ATTACHMENT && req.usage.stage == Stage::POST_RASTER;
      const bool notUsed = nodeData.sideEffect == SideEffects::None && req.usage.access == Access::READ_WRITE &&
                           req.usage.type == Usage::UNKNOWN && req.usage.stage == Stage::UNKNOWN;

      if (!usageAsExpected && !notUsed)
      {
        logerr("The node '%s' requested the back buffer (registered under '%s') with "
               "a usage that is different from a standard color attachment write! "
               "This is impossible to satisfy, as the dagor only supports rendering "
               "into the back buffer and nothing else! Disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));
        anyInvalidBackbufferRequests = true;
      }
    }
  for (const auto &[id, req] : nodeData.bindings)
    if (isBackbuffer(req.resource))
    {
      logerr("The node '%s' requested the back buffer (registered under '%s') to be bound to a shader variable! "
             "This is impossible to satisfy, as the dagor only supports rendering "
             "into the back buffer and nothing else! Disabling this node!",
        registry.knownNames.getName(nodeId), registry.knownNames.getName(req.resource));
      anyInvalidBackbufferRequests = true;
    }

  bool anyFramebufferUsageMismatches = false;
  bool invalidBackbufferPass = false;
  if (nodeData.renderingRequirements)
  {
    const auto &pass = *nodeData.renderingRequirements;

    dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> allAttachments;
    allAttachments.reserve(9);

    const auto validate = [&](ResNameId resId, Usage usage) {
      if (resId == ResNameId::Invalid)
        return;

      allAttachments.insert(resId);

      const auto it = nodeData.resourceRequests.find(resId);
      // This is an invariant
      G_ASSERT_RETURN(it != nodeData.resourceRequests.end(), );
      if (it->second.usage.type != usage)
      {
        logerr("The node '%s' requested resource '%s' as a framebuffer attachment, "
               "but the resource was requested with a different usage! "
               "This is impossible to satisfy, disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));
        anyFramebufferUsageMismatches = true;
      }
    };

    for (const auto &att : pass.colorAttachments)
      validate(att.nameId, Usage::COLOR_ATTACHMENT);
    if (pass.depthReadOnly)
      validate(pass.depthAttachment.nameId, Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE);
    else
      validate(pass.depthAttachment.nameId, Usage::DEPTH_ATTACHMENT);

    for (const auto &[resId, _] : pass.clears)
    {
      const auto it = nodeData.resourceRequests.find(resId);
      if (it == nodeData.resourceRequests.end())
      {
        logerr("The node '%s' requested resource '%s' to be cleared, "
               "but the resource was not requested at all! "
               "This is impossible to satisfy, disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));
        anyFramebufferUsageMismatches = true;
      }
      else if (allAttachments.find(resId) == allAttachments.end())
      {
        logerr("The node '%s' requested resource '%s' to be cleared, "
               "but the resource was not requested as a framebuffer attachment! "
               "This is impossible to satisfy, disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));
        anyFramebufferUsageMismatches = true;
      }
    }

    for (const auto &[from, to] : pass.resolves)
    {
      const auto fromIt = nodeData.resourceRequests.find(from);
      const auto toIt = nodeData.resourceRequests.find(to);
      if (fromIt == nodeData.resourceRequests.end())
      {
        logerr("The node '%s' requested resource '%s' to be resolved, "
               "but the resource was not requested at all! "
               "This is impossible to satisfy, disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(from));
        anyFramebufferUsageMismatches = true;
      }
      else if (toIt == nodeData.resourceRequests.end())
      {
        logerr("The node '%s' requested resource '%s' to be resolved to '%s', "
               "but the resource was not requested at all! "
               "This is impossible to satisfy, disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(from), registry.knownNames.getName(to));
        anyFramebufferUsageMismatches = true;
      }
      else if (toIt->second.usage.type != Usage::RESOLVE_ATTACHMENT)
      {
        logerr("The node '%s' requested resource '%s' to be resolved to '%s', "
               "but the resource was requested with a different usage! "
               "This is impossible to satisfy, disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(from), registry.knownNames.getName(to));
        anyFramebufferUsageMismatches = true;
      }
      else if (allAttachments.find(from) == allAttachments.end() && !fromIt->second.optional)
      {
        logerr("The node '%s' requested resource '%s' to be mandatorily resolved, "
               "but the resource was not requested as a framebuffer attachment! "
               "This is impossible to satisfy, disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(from));
        anyFramebufferUsageMismatches = true;
      }
      else
      {
        // TODO: add same checks for resources with ExternalResourceProvider creation info

        const auto &fromResData = resourceDataFor(from);
        if (const auto *resDesc = eastl::get_if<ResourceDescription>(&fromResData.creationInfo))
        {
          G_ASSERT(resDesc->resType == RES3D_TEX);
          if (!(resDesc->asTexRes.cFlags & TEXCF_SAMPLECOUNT_MASK))
          {
            logerr("The node '%s' requested single-sampled resource '%s' to be resolved to '%s'!"
                   "This is impossible to satisfy, disabling this node!",
              registry.knownNames.getName(nodeId), registry.knownNames.getName(from), registry.knownNames.getName(to));
            anyFramebufferUsageMismatches = true;
          }
        }

        const auto &toResData = resourceDataFor(to);
        if (const auto *resDesc = eastl::get_if<ResourceDescription>(&toResData.creationInfo))
        {
          G_ASSERT(resDesc->resType == RES3D_TEX);
          if (resDesc->asTexRes.cFlags & TEXCF_SAMPLECOUNT_MASK)
          {
            logerr("The node '%s' requested resource '%s' to be resolved to multisampled resource '%s'!"
                   "This is impossible to satisfy, disabling this node!",
              registry.knownNames.getName(nodeId), registry.knownNames.getName(from), registry.knownNames.getName(to));
            anyFramebufferUsageMismatches = true;
          }
        }
      }
    }

    for (const auto att : pass.colorAttachments)
      if (isBackbuffer(att.nameId))
      {
        const bool noDepth = pass.depthAttachment.nameId == ResNameId::Invalid;
        const bool noClears = pass.clears.empty();
        const bool noResolves = pass.resolves.empty();
        const bool singleColor = pass.colorAttachments.size() == 1;

        if (!noDepth || !noClears || !noResolves || !singleColor)
        {
          logerr("The node '%s' tried to use the back buffer in a way that is not allowed! "
                 "The vulkan driver is unable to use the back buffer in proper render passes, "
                 "so only a legacy fallback is possible."
                 "The back buffer can only be used as a single color attachment "
                 "with no depth attachment, clears or resolves! Disabling this node!",
            registry.knownNames.getName(nodeId));
          invalidBackbufferPass = true;
        }
      }
  }

  bool anySubtypeTagMismatches = false;

  const auto checkRequestSubtypeTag = [this, nodeId, &anySubtypeTagMismatches](ResNameId resId, const ResourceRequest &req) {
    const auto &resData = resourceDataFor(resId);
    if (resData.type != ResourceType::Blob)
      return;

    const auto &info = eastl::get<BlobDescription>(resData.creationInfo);
    if (!resourceTagsMatch(info.typeTag, req.subtypeTag))
    {
      logerr("Detected a blob type mismatch on node '%s'! "
             "It requested the blob '%s' with type '%x', while the blob was created with type '%x'! Disabling this node!",
        registry.knownNames.getName(nodeId), registry.knownNames.getName(resId), eastl::to_underlying(req.subtypeTag),
        eastl::to_underlying(info.typeTag));
      anySubtypeTagMismatches = true;
    }
  };
  for (const auto &[resId, req] : nodeData.resourceRequests)
    checkRequestSubtypeTag(resId, req);
  for (const auto &[resId, req] : nodeData.historyResourceReadRequests)
    checkRequestSubtypeTag(resId, req);

  return nodeData.execute && !anyUnfilledSlotRequests && !anyBindingTypeMismatches && !anyUsageConflictsAfterNameResolution &&
         !anyFramebufferUsageMismatches && !anyInvalidBackbufferRequests && !invalidBackbufferPass && !anySubtypeTagMismatches;
}

void RegistryValidator::validateRegistry()
{
  // We build an initial marking of invalid resources/nodes and then
  // propagate the invalid marks to dependent nodes/resources using a BFS

  auto &resourceValid = validityInfo.resourceValid;
  auto &nodeValid = validityInfo.nodeValid;

  resourceValid.clear();
  resourceValid.resize(registry.knownNames.nameCount<ResNameId>(), false);
  for (const auto resId : IdRange<ResNameId>(depData.resourceLifetimes.size()))
    resourceValid.set(resId, validateResource(resId));

  nodeValid.clear();
  nodeValid.resize(registry.knownNames.nameCount<NodeNameId>(), false);
  for (const auto nodeId : IdRange<NodeNameId>(registry.nodes.size()))
    nodeValid.set(nodeId, validateNode(nodeId));

  validateLifetimes(validityInfo);

  // We have to do an initial pass of marking resources created by
  // broken nodes as invalid.
  for (auto [nodeId, valid] : nodeValid.enumerate())
    if (!valid)
    {
      for (auto resId : registry.nodes[nodeId].createdResources)
        resourceValid[resId] = false;
      for (auto [toId, _] : registry.nodes[nodeId].renamedResources)
        resourceValid[toId] = false;
    }

  // WARNING: do NOT use bitset::set inside a framemem_region -- it reallocates!
  FRAMEMEM_VALIDATE;
  // We need a "resource -> [nodes with mandatory dependency on it]"
  // mapping that also accounts for history resources for the BFS to be quick.
  using ResToNodeMapping =
    IdIndexedMapping<ResNameId, eastl::fixed_vector<NodeNameId, 128, true, framemem_allocator>, framemem_allocator>;
  ResToNodeMapping nodesDependentOnResource(resourceValid.size());

  // We also need a mapping that says us which resources are actually
  // other resources, just renamed. This is used to disable the renamed
  // resource even if the node that does the renaming will not be disabled
  // itself due to the renaming being optional.
  using ResToResMapping = IdIndexedMapping<ResNameId, eastl::optional<ResNameId>, framemem_allocator>;
  ResToResMapping resourceRenamingDependencies(resourceValid.size());

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    for (const auto &[unresolvedResId, req] : nodeData.resourceRequests)
      if (!req.optional)
        nodesDependentOnResource[nameResolver.resolve(unresolvedResId)].push_back(nodeId);
    for (const auto &[unresolvedResId, req] : nodeData.historyResourceReadRequests)
      if (!req.optional)
        nodesDependentOnResource[nameResolver.resolve(unresolvedResId)].push_back(nodeId);

    for (const auto &[to, unresolvedFrom] : nodeData.renamedResources)
    {
      auto &dep = resourceRenamingDependencies[nameResolver.resolve(unresolvedFrom)];
      G_ASSERT(!dep.has_value());
      dep = to;
    }
  }

  // Wave of resources which are definitely invalid but who's dependent
  // nodes/resources may have not been marked as invalid yet.
  using Wave = eastl::fixed_vector<ResNameId, 64, true, framemem_allocator>;
  Wave wave;
  wave.reserve(resourceValid.size());

  // Fill in the initial wave of invalid resources
  for (const auto [resId, isValid] : resourceValid.enumerate())
    if (!isValid)
      wave.emplace_back(resId);

  // Little trick to use only 2 regions of framemem memory for the BFS.
  // Better cache locality and no heap allocations.
  Wave nextWave;
  nextWave.reserve(resourceValid.size());

  size_t swaps = 0;

  // Transitively disable all dependent nodes using a BFS
  while (!wave.empty())
  {
    nextWave.clear();

    for (auto resId : wave)
    {
      if (resourceRenamingDependencies[resId])
      {
        auto renamedToResId = *resourceRenamingDependencies[resId];

        if (resourceValid[renamedToResId])
        {
          resourceValid[renamedToResId] = false; //-V601
          nextWave.push_back(renamedToResId);
        }
      }

      for (auto nodeId : nodesDependentOnResource[resId])
      {
        if (!nodeValid[nodeId])
          continue;

        const bool resWasIntroduced = depData.resourceLifetimes[resId].introducedBy != NodeNameId::Invalid;
        const bool introducedByUs = depData.resourceLifetimes[resId].introducedBy == nodeId;

        logerr("Found a broken node '%s'! "
               "Reason: the node requested '%s' as a mandatory resource, "
               "but %s. Disabling this node!",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(resId),
          resWasIntroduced ? (introducedByUs ? "the resource itself was broken!" : "the node that produces it was broken too!")
                           : "it was never produced by any node!");

        nodeValid[nodeId] = false; //-V601

        auto invalidateRes = [&resourceValid, &nextWave](ResNameId resId) {
          if (!resourceValid[resId])
            return;

          resourceValid[resId] = false; //-V601
          nextWave.push_back(resId);
        };

        const auto &nodeData = registry.nodes[nodeId];

        for (const auto &resId : nodeData.createdResources)
          invalidateRes(resId);

        for (const auto &[to, _] : nodeData.renamedResources)
          invalidateRes(to);
      }
    }

    eastl::swap(wave, nextWave);
    ++swaps;
  }

  // Always do an even amount of swaps so that framemem lifetimes are nested.
  if (swaps % 2)
    eastl::swap(wave, nextWave);
}

const ResourceData &RegistryValidator::resourceDataFor(ResNameId resId) const
{
  return registry.resources[depData.renamingRepresentatives[nameResolver.resolve(resId)]];
}


} // namespace dabfg
