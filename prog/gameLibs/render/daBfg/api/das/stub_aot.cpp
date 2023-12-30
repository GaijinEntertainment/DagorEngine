#include <api/das/nodeEcsRegistration.h>
#include <render/daBfg/bfg.h>
#include <frontend/nodeTracker.h>
#include <runtime/runtime.h>

InitOnDemand<dabfg::Runtime, false> dabfg::Runtime::instance;
dabfg::Runtime::~Runtime() { G_ASSERT(0); }

void dabfg::detail::unregister_node(detail::NodeUid) { G_ASSERT(0); }

void dabfg::NodeTracker::registerNode(void *, NodeNameId) { G_ASSERT(0); }
void dabfg::NodeTracker::unregisterNode(NodeNameId, uint16_t) { G_ASSERT(0); }
void dabfg::NodeTracker::wipeContextNodes(void *) { G_ASSERT(0); }
void dabfg::NodeTracker::dumpRawUserGraph() const { G_ASSERT(0); }
