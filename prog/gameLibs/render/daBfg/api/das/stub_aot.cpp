// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <frontend/nodeTracker.h>
#include <runtime/runtime.h>

InitOnDemand<dabfg::Runtime, false> dabfg::Runtime::instance;
void dabfg::Runtime::wipeBlobsBetweenFrames(eastl::span<dabfg::ResNameId>) { G_ASSERT(0); }
dabfg::Runtime::~Runtime() { G_ASSERT(0); }

void dabfg::detail::unregister_node(detail::NodeUid) { G_ASSERT(0); }

void dabfg::NodeTracker::registerNode(void *, NodeNameId) { G_ASSERT(0); }
void dabfg::NodeTracker::unregisterNode(NodeNameId, uint16_t) { G_ASSERT(0); }
eastl::optional<dabfg::NodeTracker::ResourceWipeSet> dabfg::NodeTracker::wipeContextNodes(void *) { G_ASSERT_RETURN(false, {}); }
void dabfg::NodeTracker::dumpRawUserGraph() const { G_ASSERT(0); }

dabfg::NameSpace::NameSpace() { G_ASSERT(false); }
dabfg::NameSpace dabfg::root() { G_ASSERT_RETURN(false, {}); }
