// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <perfMon/dag_statDrv.h>
#include <frontend/internalRegistry.h>
#include <backend/intermediateRepresentation.h>
#include <dag/dag_vector.h>
#include <generic/dag_relocatableFixedVector.h>
#include <id/idSparseIndexedMapping.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EASTL/stack.h>


namespace dafg
{

class GraphHierarchicalMarksParser final
{
public:
  class GraphMarker
  {
  public:
    explicit GraphMarker(GraphHierarchicalMarksParser &parser);
    ~GraphMarker();

    // NOTE: nodes must be marked in execution order; marking out of order causes UB
    void markNode(intermediate::NodeIndex node_id);
    const char *getNodeDisplayName(intermediate::NodeIndex node_id) const;

  private:
    GraphHierarchicalMarksParser &parser_;
  };

public:
  GraphHierarchicalMarksParser() = default;

  void parseGraphMarks(const InternalRegistry &registry, const intermediate::Graph &graph);

  GraphMarker createGraphMarker();

private:
  static constexpr int NODE_NAMESPACE_NAME_LEN = 128;
  static constexpr size_t NODE_NAMESPACES_NUMBER = 8;

  using FixedString = eastl::fixed_string<char, NODE_NAMESPACE_NAME_LEN>;
  struct NamespaceData
  {
    NameSpaceNameId ns = NameSpaceNameId::Invalid;
    intermediate::NodeIndex id = {};
  };

  struct NodeMark
  {
    uint16_t popNum = 0;
    FixedString displayName;

    // dag::RelocatableFixedVector requires T to be relocatable, eastl::fixed_string is not
    eastl::fixed_vector<FixedString, NODE_NAMESPACES_NUMBER> newGPUEvents;
  };
  struct NodeParseData
  {
    NodeNameId nodeNameId = NodeNameId::Invalid;
    uint32_t commonNamespacesNum = 0;
    dag::RelocatableFixedVector<NamespaceData, NODE_NAMESPACES_NUMBER> popedNamespaces;
    dag::RelocatableFixedVector<NamespaceData, NODE_NAMESPACES_NUMBER> pushedNamespaces;
  };

  struct StackNodeNamespaceData
  {
    StackNodeNamespaceData(IdSparseIndexedMapping<intermediate::NodeIndex, NodeParseData> &nodes, NameSpaceNameId ns,
      intermediate::NodeIndex node_id, uint16_t push_id);

    void removedOnNode(intermediate::NodeIndex node_id);

    NameSpaceNameId ns;
    intermediate::NodeIndex nodeId;
    uint16_t pushId;

  private:
    IdSparseIndexedMapping<intermediate::NodeIndex, NodeParseData> &nodes_;
  };

private:
  void parseGraphNodes(const InternalRegistry &registry, const intermediate::Graph &graph);
  void parseNodeNamespaces(NodeNameId node_id, dag::Vector<NameSpaceNameId> &node_namespaces, const InternalRegistry &registry) const;

  void generateNodeMarks(const InternalRegistry &registry);
  NodeMark generateMark(intermediate::NodeIndex node_id, const InternalRegistry &registry) const;

  static const char *skipNamespaceInNodeName(const char *node_name, uint32_t num);

private:
  IdSparseIndexedMapping<intermediate::NodeIndex, NodeParseData> nodes_;
  IdSparseIndexedMapping<intermediate::NodeIndex, NodeMark> nodeMarks_;

  dag::Vector<StackNodeNamespaceData> currentNamespacesStack_;

  dag::Vector<NameSpaceNameId> nodeNamespaces;

#if DA_PROFILER_ENABLED
  eastl::stack<da_profiler::ScopedGPUEvent, dag::Vector<da_profiler::ScopedGPUEvent>> eventStack_;
#endif
};

} // namespace dafg
