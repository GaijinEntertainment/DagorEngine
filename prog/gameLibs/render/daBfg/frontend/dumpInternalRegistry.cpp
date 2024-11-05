// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dumpInternalRegistry.h"


void dabfg::dump_internal_registry(const InternalRegistry &registry)
{
  dump_internal_registry(
    registry, [](auto) { return true; }, [](auto) { return true; });
}

void dabfg::dump_internal_registry(const InternalRegistry &registry, NodeValidCb nodeValid, ResValidCb resourceValid)
{
  logwarn("Framegraph full user graph state dump:");
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    auto logNode = [&](NodeNameId id) { logwarn("\t\t'%s'%s", registry.knownNames.getName(id), !nodeValid(id) ? " (BROKEN)" : ""); };

    auto logRes = [&, &nodeData = nodeData](ResNameId id) {
      const auto &req = nodeData.resourceRequests.find(id)->second;
      logwarn("\t\t%s'%s'%s", req.optional ? "optional " : "", registry.knownNames.getName(id), !resourceValid(id) ? " (BROKEN)" : "");
    };

    auto dumpHelper = [](const auto &data, const char *heading, const auto &f) {
      if (data.empty())
        return;
      logwarn("\t%s", heading);
      for (const auto &d : data)
        f(d);
    };

    const bool broken = !nodeValid(nodeId);
    logwarn("Node '%s' (%spriority %d)", registry.knownNames.getName(nodeId), broken ? "BROKEN, " : "", nodeData.priority);

    dumpHelper(nodeData.followingNodeIds, "Following nodes:", logNode);

    dumpHelper(nodeData.precedingNodeIds, "Previous nodes:", logNode);

    // TODO: implement a resource description pretty printer in d3d
    // and use it here

    dumpHelper(nodeData.createdResources, "Created resources:", logRes);

    dumpHelper(nodeData.readResources, "Read resources:", logRes);

    dumpHelper(nodeData.historyResourceReadRequests, "History read resources:", [&](const auto &pair) {
      logwarn("\t\t%s'%s'%s", pair.second.optional ? "optional " : "", registry.knownNames.getName(pair.first),
        !resourceValid(pair.first) ? " (BROKEN)" : "");
    });

    dumpHelper(nodeData.modifiedResources, "Modified resources:", logRes);

    dumpHelper(nodeData.renamedResources, "Renamed resources:", [&](auto pair) {
      const bool firstBroken = !resourceValid(pair.first);
      const bool secondBroken = !resourceValid(pair.second);
      // Yes, second is the old resource, first is the new one, this is not a typo.
      logwarn("\t\t'%s'%s -> '%s'%s", registry.knownNames.getName(pair.second), secondBroken ? " (BROKEN)" : "",
        registry.knownNames.getName(pair.first), firstBroken ? " (BROKEN)" : "");
    });
  }
  logwarn("Finished dumping framegraph state.");
}
