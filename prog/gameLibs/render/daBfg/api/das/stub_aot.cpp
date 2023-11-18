#include <api/das/nodeEcsRegistration.h>
#include <render/daBfg/bfg.h>
#include <frontend/nodeTracker.h>
#include <runtime/backend.h>

InitOnDemand<dabfg::Backend, false> dabfg::Backend::instance;
dabfg::Backend::~Backend() { G_ASSERT(0); }

void dabfg::detail::unregister_node(detail::NodeUid) { G_ASSERT(0); }

void dabfg::NodeTracker::registerNode(NodeNameId) { G_ASSERT(0); }
void dabfg::NodeTracker::unregisterNode(NodeNameId, uint32_t) { G_ASSERT(0); }
void dabfg::NodeTracker::dumpRawUserGraph() const {}

static dabfg::NodeTracker *tracker = nullptr;
dabfg::NodeTracker &dabfg::NodeTracker::get()
{
  G_ASSERT(0);
  return *tracker;
}
