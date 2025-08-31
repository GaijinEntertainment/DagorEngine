// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/das/registerBlobType.h>
#include <frontend/nodeTracker.h>
#include <runtime/runtime.h>

InitOnDemand<dafg::Runtime, false> dafg::Runtime::instance;
void dafg::Runtime::wipeBlobsBetweenFrames(eastl::span<dafg::ResNameId>) { G_ASSERT(0); }
dafg::Runtime::~Runtime() { G_ASSERT(0); }

void dafg::detail::unregister_node(detail::NodeUid) { G_ASSERT(0); }

void dafg::NodeTracker::registerNode(void *, NodeNameId) { G_ASSERT(0); }
void dafg::NodeTracker::unregisterNode(NodeNameId, uint16_t) { G_ASSERT(0); }
eastl::optional<dafg::NodeTracker::ResourceWipeSet> dafg::NodeTracker::wipeContextNodes(void *) { G_ASSERT_RETURN(false, {}); }
void dafg::NodeTracker::dumpRawUserGraph() const { G_ASSERT(0); }

void dafg::TypeDb::registerInteropType(const char *, dafg::ResourceSubtypeTag, RTTI &&) { G_ASSERT(0); }
dafg::ResourceSubtypeTag dafg::TypeDb::registerForeignType(const char *, RTTI &&)
{
  G_ASSERT(0);
  return {};
}
dafg::ResourceSubtypeTag dafg::TypeDb::tagForForeign(const char *) const
{
  G_ASSERT(0);
  return {};
}

dafg::NameSpace::NameSpace() { G_ASSERT(false); }
dafg::NameSpace dafg::root() { G_ASSERT_RETURN(false, {}); }

void try_register_builtins(const das::ModuleLibrary &) {}
void dafg::detail::register_das_interop_type(const char *, dafg::ResourceSubtypeTag, RTTI &&rtti) {}
