// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graphHierarchicalMarksParser.h"

#include <debug/dag_assert.h>
#include <EASTL/algorithm.h>


namespace dafg
{

GraphHierarchicalMarksParser::GraphMarker::GraphMarker(GraphHierarchicalMarksParser &parser) : parser_(parser) {}

GraphHierarchicalMarksParser::GraphMarker::~GraphMarker()
{
#if DA_PROFILER_ENABLED
  while (!parser_.eventStack_.empty())
    parser_.eventStack_.pop();
#endif
}

void GraphHierarchicalMarksParser::GraphMarker::markNode(intermediate::NodeIndex node_id)
{
#if DA_PROFILER_ENABLED
  for (uint16_t i = 0; i < parser_.nodeMarks_[node_id].popNum; ++i)
    parser_.eventStack_.pop();

  for (const auto &eventName : parser_.nodeMarks_[node_id].newGPUEvents)
  {
    const char *name = eventName.c_str();
    parser_.eventStack_.emplace(da_profiler::get_tls_description(nullptr, 0, 0, name), name);
  }
#else
  G_UNUSED(node_id);
#endif
}

const char *GraphHierarchicalMarksParser::GraphMarker::getNodeDisplayName(intermediate::NodeIndex node_id) const
{
  return parser_.nodeMarks_[node_id].displayName.c_str();
}

void GraphHierarchicalMarksParser::parseGraphMarks(const InternalRegistry &registry, const intermediate::Graph &graph)
{
  parseGraphNodes(registry, graph);
  generateNodeMarks(registry);
}

GraphHierarchicalMarksParser::GraphMarker GraphHierarchicalMarksParser::createGraphMarker() { return GraphMarker(*this); }

void GraphHierarchicalMarksParser::parseGraphNodes(const InternalRegistry &registry, const intermediate::Graph &graph)
{
  nodes_.clear();
  currentNamespacesStack_.clear();

  nodeNamespaces.clear();
  for (intermediate::NodeIndex i : graph.nodes.keys())
  {
    const intermediate::Node &node = graph.nodes[i];
    if (!node.frontendNode.has_value())
      continue;

    const NodeNameId nodeNameId = node.frontendNode.value();
    parseNodeNamespaces(nodeNameId, nodeNamespaces, registry);

    size_t commonLen = 0;
    while (commonLen < currentNamespacesStack_.size() && commonLen < nodeNamespaces.size() &&
           currentNamespacesStack_[commonLen].ns == nodeNamespaces[commonLen])
      ++commonLen;

    NodeParseData *pNodeData = nodes_.emplaceAt(i);
    G_ASSERT(pNodeData);
    NodeParseData &nodeData = *pNodeData;

    nodeData.nodeNameId = nodeNameId;
    nodeData.commonNamespacesNum = commonLen;

    while (currentNamespacesStack_.size() > commonLen)
    {
      nodeData.popedNamespaces.emplace_back(NamespaceData{
        .ns = currentNamespacesStack_.back().ns,
        .id = currentNamespacesStack_.back().nodeId,
      });

      currentNamespacesStack_.back().removedOnNode(i);
      currentNamespacesStack_.pop_back();
    }

    for (size_t level = commonLen; level < nodeNamespaces.size(); ++level)
    {
      const NameSpaceNameId ns = nodeNamespaces[level];

      const uint16_t pushId = nodeData.pushedNamespaces.size();

      nodeData.pushedNamespaces.emplace_back(NamespaceData{.ns = ns});
      currentNamespacesStack_.emplace_back(StackNodeNamespaceData(nodes_, ns, i, pushId));
    }
  }

  const auto endIndex = static_cast<intermediate::NodeIndex>(graph.nodes.totalKeys() - 1);
  while (!currentNamespacesStack_.empty())
  {
    currentNamespacesStack_.back().removedOnNode(endIndex);
    currentNamespacesStack_.pop_back();
  }
}

void GraphHierarchicalMarksParser::parseNodeNamespaces(NodeNameId node_id, dag::Vector<NameSpaceNameId> &node_namespaces,
  const InternalRegistry &registry) const
{
  node_namespaces.clear();
  for (NameSpaceNameId ns = registry.knownNames.getParent(node_id); ns != registry.knownNames.root();
       ns = registry.knownNames.getParent(ns))
    node_namespaces.emplace_back(ns);
  eastl::reverse(node_namespaces.begin(), node_namespaces.end());
}

void GraphHierarchicalMarksParser::generateNodeMarks(const InternalRegistry &registry)
{
  nodeMarks_.clear();

  for (auto i : nodes_.keys())
    nodeMarks_.emplaceAt(i, generateMark(i, registry));
}

GraphHierarchicalMarksParser::NodeMark GraphHierarchicalMarksParser::generateMark(intermediate::NodeIndex node_id,
  const InternalRegistry &registry) const
{
  NodeMark mark;

  const NodeParseData &data = nodes_[node_id];

  const auto &popedNamespaces = data.popedNamespaces;
  for (size_t i = 0; i < data.popedNamespaces.size(); ++i)
  {
    // skip namespace with only one node
    if (popedNamespaces[i].id + 1 == node_id)
      continue;

    ++mark.popNum;

    // skip squished namespaces
    while (i + 1 < popedNamespaces.size() && popedNamespaces[i].id == popedNamespaces[i + 1].id)
      ++i;
  }

  uint32_t nodeDepth = data.commonNamespacesNum;

  const auto &pushedNamespaces = data.pushedNamespaces;
  for (size_t i = 0; i < pushedNamespaces.size(); ++i)
  {
    // skip namespace with only one node
    if (pushedNamespaces[i].id == node_id + 1)
      break;

    ++nodeDepth;

    FixedString eventName = registry.knownNames.getShortName(pushedNamespaces[i].ns);
    eventName += '/';

    // skip squished namespaces
    while (i + 1 < pushedNamespaces.size() && pushedNamespaces[i].id == pushedNamespaces[i + 1].id)
    {
      ++nodeDepth;

      eventName += registry.knownNames.getShortName(pushedNamespaces[++i].ns);
      eventName += '/';
    }

    mark.newGPUEvents.emplace_back(eastl::move(eventName));
  }

  mark.displayName = skipNamespaceInNodeName(registry.knownNames.getName(data.nodeNameId), nodeDepth);

  return mark;
}

const char *GraphHierarchicalMarksParser::skipNamespaceInNodeName(const char *node_name, uint32_t num)
{
  G_ASSERT(node_name);

  if (*node_name == '/')
    ++num;

  for (uint32_t i = 0; i < num; ++i)
  {
    const char *slash = strchr(node_name, '/');
    if (slash)
      node_name = slash + 1;
  }

  return node_name;
}

GraphHierarchicalMarksParser::StackNodeNamespaceData::StackNodeNamespaceData(
  IdSparseIndexedMapping<intermediate::NodeIndex, NodeParseData> &nodes, NameSpaceNameId ns, intermediate::NodeIndex node_id,
  uint16_t push_id) :
  ns(ns), nodes_(nodes), nodeId(node_id), pushId(push_id)
{}

void GraphHierarchicalMarksParser::StackNodeNamespaceData::removedOnNode(intermediate::NodeIndex node_id)
{
  nodes_[nodeId].pushedNamespaces[pushId].id = node_id;
}

} // namespace dafg
