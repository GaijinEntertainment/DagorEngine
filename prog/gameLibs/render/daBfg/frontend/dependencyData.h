// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_vector.h>

#include <render/daBfg/detail/nodeNameId.h>
#include <render/daBfg/detail/resNameId.h>
#include <id/idIndexedMapping.h>


namespace dabfg
{

// NOTE: All NameIds present in this structure are guaranteed to
// refer to existing nodes by construction. INVALID_NAME_ID
// means "no node" in this case, as not all resources go through
// the complete lifetime cycle of produce-modify-read-consume.
struct VirtualResourceLifetime
{
  // Either produced or renamed from something else by this node
  NodeNameId introducedBy = NodeNameId::Invalid;

  // Chain of non-renaming modification
  eastl::fixed_vector<NodeNameId, 32> modificationChain;

  // List of nodes that want to read this resource before it gets consumed
  eastl::fixed_vector<NodeNameId, 32> readers;

  // A node that renames this resource (mutually exclusive with nextFrameReaders!!!)
  NodeNameId consumedBy = NodeNameId::Invalid;
};

struct DependencyData
{
  // Note that this mapping is built w.r.t. resolved resource name ids
  using ResourceLifetimes = IdIndexedMapping<ResNameId, VirtualResourceLifetime>;
  ResourceLifetimes resourceLifetimes;

  // First element in each modifying rename sequence
  using RenamingRepresentatives = IdIndexedMapping<ResNameId, ResNameId>;
  RenamingRepresentatives renamingRepresentatives;
  // Mapping of resource -> what it gets renamed into
  using RenamingChains = IdIndexedMapping<ResNameId, ResNameId>;
  RenamingChains renamingChains;
};

} // namespace dabfg
